#include <nds.h>
#include <fat.h>
#include "Text.h"
#include "main.h"

Text::Text()
{
	cachenext = 0;
	fontfilename = FONTFILENAME;
	invert = false;
	justify = false;
	pixelsize = PIXELSIZE;
	screenleft = (u16*)BG_BMP_RAM_SUB(0);
	screenright = (u16*)BG_BMP_RAM(0);
	screen = screenleft;
	cachemap.clear();
}

int Text::InitDefault(void) {
	if (FT_Init_FreeType(&library)) return 1;
	if (FT_New_Face(library, fontfilename.c_str(), 0, &face)) return 2;
	FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	FT_Set_Pixel_Sizes(face, 0, pixelsize);
	screen = screenleft;
	InitPen();
	return(0);
}

FT_GlyphSlot Text::CacheGlyph(u16 ucs)
{
	//FT_UInt index = FT_Get_Char_Index(face, ucs);
	FT_UInt index = ucs;
	FT_Load_Char(face, index,
		FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL);
	FT_GlyphSlot src = face->glyph;
	FT_GlyphSlot dst = &glyphs[cachenext];
	int x = src->bitmap.rows;
	int y = src->bitmap.width;
	dst->bitmap.buffer = new unsigned char[x*y];
	memcpy(dst->bitmap.buffer, src->bitmap.buffer, x*y);
	dst->bitmap.rows = src->bitmap.rows;
	dst->bitmap.width = src->bitmap.width;
	dst->bitmap_top = src->bitmap_top;
	dst->bitmap_left = src->bitmap_left;
	dst->advance = src->advance;

	map<u16,u16>::iterator it;
	for(it = cachemap.begin(); it != cachemap.end(); it++)
	{
		if(it->second == cachenext)
			cachemap.erase(it);
	}
	cachemap.insert( pair<u16,u16>(ucs,cachenext));

	// cache is a ring buffer.
	cachenext++;
	if(cachenext >= CACHESIZE) cachenext = 0;

	return dst;
}

void Text::ClearScreen()
{
	if(invert) memset((void*)screen,0,PAGE_WIDTH*PAGE_HEIGHT*4);
	else memset((void*)screen,255,PAGE_WIDTH*PAGE_HEIGHT*4);
}

void Text::ClearRect(u16 xl, u16 yl, u16 xh, u16 yh)
{
	u16 clearcolor;
	if(invert) clearcolor = RGB15(0,0,0) | BIT(15);
	else clearcolor = RGB15(31,31,31) | BIT(15);
	for(u16 y=yl; y<yh; y++) {
		for(u16 x=xl; x<xh; x++) {
			screen[y*PAGE_WIDTH+x] = clearcolor;
		}
	}
}

u8 Text::GetStringWidth(const char *txt)
{
	u8 width = 0;
	const char *c;
	for(c = txt; c != NULL; c++)
	{
		u16 ucs;
		GetCharCode(c, &ucs);
		width += GetAdvance(ucs);
	}
	return width;
}	


u8 Text::GetCharCode(const char *utf8, u16 *ucs) {
	// given a UTF-8 encoding, fill in the Unicode/UCS code point.
	// returns the bytelength of the encoding, for advancing
	// to the next character.
	// returns 0 if encoding could not be translated.
	// TODO - handle 4 byte encodings.

	if (utf8[0] < 0x80) { // ASCII
		*ucs = utf8[0];
		return 1;

	} else if (utf8[0] > 0xc1 && utf8[0] < 0xe0) { // latin
		*ucs = ((utf8[0]-192)*64) + (utf8[1]-128);
		return 2;

	} else if (utf8[0] > 0xdf && utf8[0] < 0xf0) { // asian
		*ucs = (utf8[0]-224)*4096 + (utf8[1]-128)*64 + (utf8[2]-128);
		return 3;

	} else if (utf8[0] > 0xef) { // rare
		return 4;

	}
	return 0;
}

u8 Text::GetHeight() {
	return (face->size->metrics.height >> 6);
}

void Text::GetPen(u16 *x, u16 *y) {
	*x = pen.x;
	*y = pen.y;
}

void Text::SetPen(u16 x, u16 y) {
	pen.x = x;
	pen.y = y;
}

void Text::SetInvert(bool state) {
	invert = state;
}

bool Text::GetInvert() {
	return invert;
}

u8 Text::GetPenX() {
	return pen.x;
}

u8 Text::GetPenY() {
	return pen.y;
}

u8 Text::GetPixelSize()
{
	return pixelsize;
}

u16* Text::GetScreen()
{
	return screen;
}

void Text::SetPixelSize(u8 size)
{
	if (!size) {
		FT_Set_Pixel_Sizes(face, 0, PIXELSIZE);
		pixelsize = PIXELSIZE;
	} else {
		FT_Set_Pixel_Sizes(face, 0, size);
		pixelsize = size;
	}
	cachemap.clear();
}

void Text::SetScreen(u16 *inscreen)
{
	screen = inscreen;
}

u8 Text::GetAdvance(u16 ucs) {
	map<u16,u16>::iterator it;
	it = cachemap.find(ucs);
	if(it != cachemap.end())
		return glyphs[it->second].advance.x >> 6;
	return CacheGlyph(ucs)->advance.x >> 6;

	//FT_UInt index = FT_Get_Char_Index(face,ucs);
	//FT_Load_Char(face, ucs, FT_LOAD_DEFAULT);
	//return face->glyph->advance.x >> 6;
}

void Text::InitPen(void) {
	pen.x = MARGINLEFT;
	pen.y = MARGINTOP + (face->size->metrics.height >> 6);
}

void Text::PrintChar(u16 ucs) {
	// Draw a character for the given UCS codepoint,
	// into the current screen buffer at the current pen position.

	// Consult the cache for glyph data; ask freetype on a cache miss.
	FT_GlyphSlot glyph;
	map<u16,u16>::iterator it;
	it = cachemap.find(ucs);
	if (it != cachemap.end()) glyph = &glyphs[it->second];
	else glyph = CacheGlyph(ucs);

	//FT_UInt index = FT_Get_Char_Index(face,ucs);
	//FT_Load_Char(face, ucs, FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL);
	//glyph = face->glyph;

	FT_Bitmap bitmap = glyph->bitmap;
	u16 bx = glyph->bitmap_left;
	u16 by = glyph->bitmap_top;
	u16 gx, gy;
	for (gy=0; gy<bitmap.rows; gy++) {
		for (gx=0; gx<bitmap.width; gx++) {
			u16 a = bitmap.buffer[gy*bitmap.width+gx];
			if (a) {
				u16 sx = (pen.x+gx+bx);
				u16 sy = (pen.y+gy-by);
				int l;
				if (invert) l = a >> 3;
				else l = (255-a) >> 3;
				screen[sy*SCREEN_WIDTH+sx] = RGB15(l,l,l) | BIT(15);
			}
		}
	}
	pen.x += glyph->advance.x >> 6;
}

bool Text::PrintNewLine(void) {
	pen.x = MARGINLEFT;
	int height = face->size->metrics.height >> 6;
	int y = pen.y + height + LINESPACING;
	if (y > (PAGE_HEIGHT - MARGINBOTTOM)) {
		if (screen == screenleft)
		{
			screen = screenright;
			pen.y = MARGINTOP + height;
			return true;
		} 
		else
			return false;
	}
	else
	{
		pen.y += height + LINESPACING;
		return true;
	}
}

void Text::PrintString(const char *string) {
	// draw a character string starting at the pen position.
	u8 i;
	for (i=0;i<strlen((char *)string);i++) {
		u16 c = string[i];
		if (c == '\n') PrintNewLine();
		else {
			if (c > 127) {
				/** this guy is multibyte UTF-8. **/
				i+=GetCharCode(&(string[i]),&c);
				i--;
			}
			PrintChar(c);
		}
	}
}

void Text::PrintStatusMessage(const char *msg)
{
	u16 x,y;
	u16 *s = screen;
	GetPen(&x,&y);
	screen = screenleft;	
	SetPen(10,10);
	PrintString(msg);
	screen = s;
	SetPen(x,y);
}


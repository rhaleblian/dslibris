/* 

dslibris - an ebook reader for the Nintendo DS.

 Copyright (C) 2007-2008 Ray Haleblian

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include <iostream>
#include <sstream>
#include <sys/param.h>

#include "nds.h"
#include "fat.h"
#include "string.h"

#include "app.h"
#include "ft.h"
#include "log.h"
#include "main.h"
#include "version.h"
#include "text.h"

extern char msg[];
std::stringstream ss;

void Text::CopyScreen(u16 *src, u16 *dst) {
	memcpy(src, dst, display.width * display.height * sizeof(u16));
}

Text::Text()
{
	display.height = PAGE_HEIGHT;
	display.width = PAGE_WIDTH;
	filenames[TEXT_STYLE_REGULAR] = FONTREGULARFILE;
	filenames[TEXT_STYLE_BOLD] = FONTBOLDFILE;
	filenames[TEXT_STYLE_ITALIC] = FONTITALICFILE;
	filenames[TEXT_STYLE_BROWSER] = FONTBROWSERFILE;
	filenames[TEXT_STYLE_SPLASH] = FONTSPLASHFILE;
	screenleft = (u16*)BG_BMP_RAM_SUB(0);
	screenright = (u16*)BG_BMP_RAM(0);
	offscreen = new u16[display.width * display.height];
	margin.left = MARGINLEFT;
	margin.right = MARGINRIGHT;
	margin.top = MARGINTOP;
	margin.bottom = MARGINBOTTOM;
	bgcolor.r = 31;
	bgcolor.g = 31;
	bgcolor.b = 15;
	usebgcolor = false;
	invert = false;
	justify = false;
	linespacing = 0;
	ftc = false;
	initialized = false;
	imagetype.face_id = (FTC_FaceID)&face_id;
	imagetype.flags = FT_LOAD_DEFAULT; 
	imagetype.height = pixelsize;
	imagetype.width = 0;

	// Rendering state.
	hit = false;
	linebegan = false;
	codeprev = 0;
	bold = false;
	italic = false;
	face = NULL;
	style = TEXT_STYLE_REGULAR;
	pixelsize = PIXELSIZE;
	screen = screenleft;

	// Statistics.
	stats_hits = 0;
	stats_misses = 0;

	ss.clear();
}

Text::~Text()
{
	// framebuffers
	if (offscreen) free(offscreen);
	
	// homemade cache
	ClearCache();
	for(map<FT_Face, Cache*>::iterator iter = textCache.begin();
		iter != textCache.end(); iter++) {
		delete iter->second;
	}
	textCache.clear();
	
	// FreeType
	for (map<u8, FT_Face>::iterator iter = faces.begin(); iter != faces.end(); iter++) {
		FT_Done_Face(iter->second);
	}
	FT_Done_FreeType(library);
}

static FT_Error
TextFaceRequester(    FTC_FaceID   face_id,
                      FT_Library   library,
                      FT_Pointer   request_data,
                      FT_Face     *aface )
{
	TextFace face = (TextFace) face_id;   // simple typecase
	return FT_New_Face( library, face->file_path, face->face_index, aface );
}

FT_Error Text::InitFreeTypeCache(void) {
	//! Use FreeType's cache manager. borken!

	auto error = FT_Init_FreeType(&library);
	if(error) return error;
	error = FTC_Manager_New(library,0,0,0,
		&TextFaceRequester,NULL,&cache.manager);
	if(error) return error;
	error = FTC_ImageCache_New(cache.manager,&cache.image);
	if(error) return error;
	error = FTC_SBitCache_New(cache.manager,&cache.sbit);
	if(error) return error;
	error = FTC_CMapCache_New(cache.manager,&cache.cmap);
	if(error) return error;

	sprintf(face_id.file_path, "%s/%s", FONTDIR, filenames[TEXT_STYLE_REGULAR].c_str());
	face_id.face_index = 0;
	sprintf(msg, "%s %s %d\n", filenames[TEXT_STYLE_REGULAR].c_str(), face_id.file_path, face_id.face_index);
	Log(msg);
	error =	FTC_Manager_LookupFace(cache.manager, (FTC_FaceID)&face_id, &faces[TEXT_STYLE_REGULAR]);
	if(error) return error;

	// FT_Select_Charmap(GetFace(TEXT_STYLE_REGULAR), FT_ENCODING_UNICODE);
	// charmap_index = FT_Get_Charmap_Index(GetFace(TEXT_STYLE_REGULAR)->charmap);

	screen = screenleft;
	InitPen();
	initialized = true;
	return 0;
}

FT_Error Text::CreateFace(int style) {
	std::string path = std::string(FONTDIR) + "/" + filenames[style];
	FT_Error err = FT_New_Face(library, path.c_str(), 0, &face);
	if (!err)
		faces[style] = face;
	else {
		faces[style] = nullptr;
	}
	return err;
}

int Text::InitHomemadeCache(void) {
	//! Use our own cheesey glyph cache.
	FT_Error err;

	err = FT_Init_FreeType(&library);
	if (err) return err;
	
	err = CreateFace(TEXT_STYLE_BROWSER);
	if (err) return err;
	err |= CreateFace(TEXT_STYLE_SPLASH);
	err |= CreateFace(TEXT_STYLE_REGULAR);
	err |= CreateFace(TEXT_STYLE_ITALIC);
	if (err)
		faces[TEXT_STYLE_ITALIC] = faces[TEXT_STYLE_REGULAR];
	err = CreateFace(TEXT_STYLE_BOLD);
	if (err)
		faces[TEXT_STYLE_BOLD] = faces[TEXT_STYLE_REGULAR];

	std::map<u8, FT_Face>::iterator iter;
	for (iter = faces.begin(); iter != faces.end(); iter++) {
		FT_Set_Pixel_Sizes(iter->second, 0, pixelsize);
		textCache.insert(make_pair(iter->second, new Cache()));
	}

	screen = screenleft;
	face = faces[style];
	ClearCache();
	InitPen();
	initialized = true;
	return 0;
}

int Text::Init()
{
	if(ftc)
		return InitFreeTypeCache();
	else
		return InitHomemadeCache();
	return 0;
}

void Text::ReportFace(FT_Face face)
{
	sprintf(msg, "%s %s\n", face->family_name, face->style_name);
	Log(msg);
	
	// sprintf(msg, "faces %ld\n", face->num_faces);
	// sprintf(msg, "glyphs %ld\n", face->num_glyphs);
	// sprintf(msg, "fixed-sizes %d\n", face->num_fixed_sizes);
	// for (int i=0;i<face->num_fixed_sizes;i++)
	// {
	// 	sprintf(msg, " w %d h %d\n",
	// 		face->available_sizes[i].width,
	// 		face->available_sizes[i].height);
	// 	Log(msg);
	// }
}

void Text::Begin()
{
	bold = false;
	italic = false;
	linebegan = false;
	stats_hits = 0;
	stats_misses = 0;
	hit = false;
}

void Text::End() {}

int Text::CacheGlyph(u32 ucs)
{
	return CacheGlyph(ucs, TEXT_STYLE_REGULAR);
}

int Text::CacheGlyph(u32 ucs, u8 style)
{
	return CacheGlyph(ucs, GetFace(style));
}

int Text::CacheGlyph(u32 ucs, FT_Face face)
{
	//! Cache glyph at ucs if there's space.

	//! Does not check if this is a duplicate entry;
	//! The caller should have checked first.

	if(textCache[face]->cacheMap.size() == CACHESIZE) return -1;

	FT_Select_Charmap(GetFace(TEXT_STYLE_REGULAR), FT_ENCODING_UNICODE);
	FT_Load_Char(face, ucs,
		FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL);
	FT_GlyphSlot src = face->glyph;
	// TODO - why?
	//FT_GlyphSlot dst = &textCache[face]->glyphs[textCache[face]->cachenext];
	FT_GlyphSlot dst = new FT_GlyphSlotRec;
	int x = src->bitmap.rows;
	int y = src->bitmap.width;
	dst->bitmap.buffer = new unsigned char[x*y];
	memcpy(dst->bitmap.buffer, src->bitmap.buffer, x*y);
	dst->bitmap.rows = src->bitmap.rows;
	dst->bitmap.width = src->bitmap.width;
	dst->bitmap_top = src->bitmap_top;
	dst->bitmap_left = src->bitmap_left;
	dst->advance = src->advance;
	//textCache[face]->cache_ucs[textCache[face]->cachenext] = ucs;
	textCache[face]->cacheMap.insert(make_pair(ucs, dst));
	//textCache[face]->cachenext++;
	//return textCache[face]->cachenext-1;
	return ucs;
}

FT_UInt Text::GetGlyphIndex(u32 ucs)
{
	//! Given a UCS codepoint, return where it is in the charmap, by index.
	
	//! Only has effect when FT cache mode is enabled,
	//! and FT cache mode is borken.
	if(!ftc) return ucs;
	return FTC_CMapCache_Lookup(cache.cmap,(FTC_FaceID)&face_id,
		charmap_index,ucs);
}

int Text::GetGlyphBitmap(u32 ucs, FTC_SBit *sbit, FTC_Node *anode)
{
	//! Given a UCS code, fills sbit and anode.
	
	//! Returns nonzero on error.
	imagetype.flags = FT_LOAD_DEFAULT|FT_LOAD_RENDER;
	return FTC_SBitCache_Lookup(cache.sbit,&imagetype,
		GetGlyphIndex(ucs),sbit,anode);
}

FT_GlyphSlot Text::GetGlyph(u32 ucs, int flags)
{
	return GetGlyph(ucs, flags, face);
}

FT_GlyphSlot Text::GetGlyph(u32 ucs, int flags, u8 style)
{
	return GetGlyph(ucs, flags, GetFace(style));
}

FT_GlyphSlot Text::GetGlyph(u32 ucs, int flags, FT_Face face)
{
	if(ftc) return NULL;

#if 0
	for(int i=0;i<textCache[face]->cachenext;i++)
		if(textCache[face]->cache_ucs[i] == ucs)
			return &textCache[face]->glyphs[i];
#endif	

	// map<u16,FT_GlyphSlot>::iterator iter = textCache[face]->cacheMap.find(ucs);
	
	// if (iter != textCache[face]->cacheMap.end()) {
	// 	stats_hits++;
	// 	hit = true;
	// 	return iter->second;
	// }
	
	// stats_misses++;
	// hit = false;
	// int i = CacheGlyph(ucs, face);
	// if (i > -1)
	// 	return textCache[face]->cacheMap[ucs];

	FT_Load_Char(face, ucs, flags);
	return face->glyph;
}

void Text::ClearCache()
{
	 for (map<u8, FT_Face>::iterator iter = faces.begin(); iter != faces.end(); iter++) {
		 ClearCache(iter->second);
	 }
}

void Text::ClearCache(u8 style)
{
	ClearCache(GetFace(style));
}

void Text::ClearCache(FT_Face face)
{
	//textCache[face]->cachenext = 0;
	map<u16, FT_GlyphSlot>::iterator iter;   
	for(iter = textCache[face]->cacheMap.begin(); iter != textCache[face]->cacheMap.end(); iter++) {
		delete iter->second;
	}

	textCache[face]->cacheMap.clear();
}

void Text::ClearScreen()
{
// 	if(invert) memset((void*)screen,0,PAGE_WIDTH*PAGE_HEIGHT*4);
// 	else memset((void*)screen,255,PAGE_WIDTH*PAGE_HEIGHT*4);
	u16 color = ARGB16(1, 15, 15, 15);
	for (int y = 0; y < display.height; y++) {
		for (int x = 0; x < display.width; x++) {
			screen[y*display.width+x] = color;
		}
	}
}

void Text::ClearRect(u16 xl, u16 yl, u16 xh, u16 yh)
{
	u16 clearcolor;
	if(invert) clearcolor = RGB15(0,0,0) | BIT(15);
	else clearcolor = RGB15(31,31,31) | BIT(15);
	//uint word = (clearcolor << 16) | clearcolor;
	for(u16 y=yl; y<yh; y++) {
//		memcpy((void*)screen[y*display.height+xl],(void*)word,xh-xl/2);
		for(u16 x=xl; x<xh; x++) {
			// FIXME: crashes on hw
			screen[y*display.height+x] = clearcolor;
		}
	}
}

u8 Text::GetFontCount() {
	u8 count = 0;
	if (faces[TEXT_STYLE_BROWSER]) count++;
	if (faces[TEXT_STYLE_SPLASH]) count++;
	if (faces[TEXT_STYLE_REGULAR]) count++;
	if (faces[TEXT_STYLE_ITALIC]) count++;
	if (faces[TEXT_STYLE_BOLD]) count++;
	return count;
}

u8 Text::GetStringWidth(const char *txt, u8 style)
{
	return GetStringWidth(txt, GetFace(style));
}

u8 Text::GetStringWidth(const char *s, FT_Face face)
{
	//! Return total advance in pixels.
	u8 width = 0;
	const char *c;
	for(c = s; c != NULL; c++)
	{
		u32 ucs = 0;
		GetCharCode(c, &ucs);
		width += GetAdvance(ucs, face);
	}
	return width;
}

u8 Text::GetCharCountInsideWidth(const char *txt, u8 style, u8 pixels) {
	u8 n = 0;
	u8 width = 0;
	const char *c;
	for(c = txt; c != NULL; c++)
	{
		u32 ucs = 0;
		GetCharCode(c, &ucs);
		width += GetAdvance(ucs, GetFace(style));
		if (width > pixels) return n;
		n++;
	}
	return n;
}

u8 Text::GetCharCode(const char *utf8, u32 *ucs) {
	//! Given a UTF-8 encoding, fill in the Unicode/UCS code point.
	//! Return the bytelength of the encoding, for advancing
	//! to the next character; 0 if encoding could not be translated.
	
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
	return (GetFace(style)->size->metrics.height >> 6);
}

void Text::GetPen(u16 *x, u16 *y) {
	*x = pen.x;
	*y = pen.y;
}

void Text::SetPen(u16 x, u16 y) {
	pen.x = x;
	pen.y = y;
}

void Text::GetPen(u16 &x, u16 &y) {
	x = pen.x;
	y = pen.y;
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
	if(ftc) {
		imagetype.height = size;
		imagetype.width = size;
		pixelsize = size;
		return;
	}

	for (map<u8, FT_Face>::iterator iter = faces.begin(); iter != faces.end(); iter++) {
		if (!size)
			FT_Set_Pixel_Sizes(iter->second, 0, PIXELSIZE);
		else
			FT_Set_Pixel_Sizes(iter->second, 0, size);
	}
	
	ClearCache();
}

void Text::SetScreen(u16 *inscreen)
{
	screen = inscreen;
}

u8 Text::GetAdvance(u32 ucs) {
	return GetAdvance(ucs, GetFace(style));
}

u8 Text::GetAdvance(u32 ucs, u8 astyle) {
	return GetAdvance(ucs, GetFace(astyle));
}

u8 Text::GetAdvance(u32 ucs, FT_Face face) {
	//! Return glyph advance in pixels.
	//! All other flavours of GetAdvance() call this one.

//	FT_Fixed padvance;
//	error = FT_Get_Advance(face, GetGlyphIndex(ucs), NULL, &padvance);
//	return padvance >> 6;
	
	if(!ftc)
		// Caches this glyph if possible.
		return GetGlyph(ucs, FT_LOAD_DEFAULT, face)->advance.x >> 6;

	imagetype.flags = FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP;

#if 0
	error = FTC_SBitCache_Lookup(cache.sbit,&imagetype,
		GetGlyphIndex(ucs),&sbit,NULL);
	return sbit->xadvance;
#endif
	FT_Glyph glyph;
	FTC_ImageType type = &imagetype;
	error = FTC_ImageCache_Lookup(cache.image,type,GetGlyphIndex(ucs),&glyph,NULL);
	return (glyph->advance).x;
}

int Text::GetStringAdvance(const char *s) {
	int advance = 0;
	for(unsigned int i=0;i<strlen(s);i++) {
		advance += GetAdvance(s[i]);
	}
	return advance;
}

bool Text::GetFontName(std::string &s) {
	const char *name = FT_Get_Postscript_Name(GetFace(TEXT_STYLE_REGULAR));
	if(!name)
		return false;
	else {
		s = name;
		return true;
	}
}

void Text::InitPen(void) {
	pen.x = margin.left;
	pen.y = margin.top + GetHeight();
}

u8 Text::PrintChar(u32 ucs)
{
	return PrintChar(ucs, GetFace(style));
}

u8 Text::PrintChar(u32 ucs, u8 astyle) {
	return PrintChar(ucs, GetFace(astyle));
}

u8 Text::PrintChar(u32 ucs, FT_Face face) {
	// Draw a character for the given UCS codepoint,
	// into the current screen buffer at the current pen position.

	// DEBUG Mark the pen position.
	// screen[pen.y*display.height+pen.x] = ARGB16(1, 31, 0, 0);

	u16 bx, by, width, height = 0;
	FT_Byte *buffer = nullptr;
	FT_UInt advance = 0;
	FT_Glyph glyph;	
	
	face = faces[TEXT_STYLE_REGULAR];
	error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	u16 glyph_index = FT_Get_Char_Index(face, ucs);
	error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT|FT_LOAD_TARGET_NORMAL);
	error = FT_Get_Glyph(face->glyph, &glyph);
	
	if (glyph->format != FT_GLYPH_FORMAT_BITMAP)
		error = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 0);
	printf("%ul %ul %ul %ul\n",
		FT_GLYPH_FORMAT_BITMAP, FT_GLYPH_FORMAT_OUTLINE, FT_GLYPH_FORMAT_COMPOSITE,
		glyph->format);
	FT_BitmapGlyph glyph_bitmap = (FT_BitmapGlyph)glyph;
	bx = glyph_bitmap->left;
	by = glyph_bitmap->top;
	FT_Bitmap bitmap = glyph_bitmap->bitmap;
	width = bitmap.width;
	height = bitmap.rows;
	buffer = bitmap.buffer;
	printf("%d %d %d %d\n", bx, by, width, height);
	advance = glyph->advance.x >> 6;

	// render to framebuffer.

	u16 sx, sy, gx, gy;
	for (gy=0; gy<height; gy++) {
		for (gx=0; gx<width; gx++) {
			u8 a = buffer[gy*width+gx];
			if (a) {
				sx = (pen.x+gx+bx);
				sy = (pen.y+gy-by);
				// screen[sy*display.height+sx] = ARGB16(1, 0, 0, 0);
			}
		}
	}

	pen.x += advance;
	codeprev = ucs;
	// FT_Done_Glyph(glyph);
	
	return sx;
}

bool Text::PrintNewLine(void) {
	//! Render a newline at the current position.
	pen.x = margin.left;
	int height = GetHeight();
	int y = pen.y + height + linespacing;
	if (y > (display.height - margin.bottom)) {
		if (screen == screenleft)
		{
			screen = screenright;
			pen.y = margin.top + height;
			return true;
		}
		else
			return false;
	}
	else
	{
		pen.y += height + linespacing;
		return true;
	}
}

void Text::PrintString(const char *s) {
	//! Render a character string starting at the pen position.
	PrintString(s, TEXT_STYLE_BROWSER);
}

void Text::PrintString(const char *s, u8 style) {
	//! Render a character string starting at the pen position.
	PrintString(s, GetFace(style));
}

void Text::PrintString(const char *s, FT_Face face) {
	//! Render a character string starting at the pen position.
	//Log(s);
	//u32 clast = 0;
	u8 i=0;
	while(i<strlen((char*)s)) {
		u32 c = s[i];
		if (c == '\n') {
			PrintNewLine();
			i++;
		} else {
			i+=GetCharCode(&(s[i]),&c);
			PrintChar(c, face);
			//clast = c;
		}
	}
}

void Text::PrintStats() {
	//! Tell log how well we're caching.
	sprintf(msg, "info: %d cache hits.\n", stats_hits);
	Log(msg);
	sprintf(msg, "info: %d cache misses.\n", stats_misses);
	Log(msg);
}

void Text::PrintStatusMessage(const char *msg)
{
	//! Render a one-liner message on the left screen.
	u16 x,y;
	GetPen(&x,&y);
	u16 *s = screen;
	int ps = GetPixelSize();
	bool invert = GetInvert();
	
	screen = screenleft;
	SetInvert(invert);
	SetPixelSize(10);
	SetPen(16, PAGE_HEIGHT-32);
	PrintString(msg);

	SetInvert(invert);
	SetPixelSize(ps);
	screen = s;
	SetPen(x,y);
}

void Text::ClearScreen(u16 *screen, u8 r, u8 g, u8 b)
{
	for (int i=0;i<PAGE_HEIGHT*PAGE_HEIGHT;i++)
		screen[i] = RGB15(r,g,b) | BIT(15);
}

void Text::PrintSplash(u16 *screen)
{
	u8 size = GetPixelSize();
	u16* s = GetScreen();
	
	SetScreen(screen);
	drawstack(screen);
	sprintf(msg,"%s",VERSION);
	PrintStatusMessage(msg);
	
	SetPixelSize(size);
	SetInvert(invert);
	SetScreen(s);
	
	swiWaitForVBlank();
}

void Text::SetFontFile(const char *filename, u8 style)
{
	if(!strcmp(filenames[style].c_str(),filename)) return;
	filenames[style] = filename;
	if(initialized) ClearCache(style);
}

string Text::GetFontFile(u8 style)
{
	return filenames[style];
}

bool Text::SetFace(u8 astyle)
{
	style = astyle;
	face = faces[style];
	return true;
}

/*
FT_Face Text::GetFace(u8 style)
{
	return face;

	map<u8, FT_Face>::iterator iter = faces.find(style);
	if (iter != faces.end())
		return iter->second;
	else
		return faces[TEXT_STYLE_REGULAR];
}
*/

const char* ErrorString(u8 code) {
	std::vector<std::string> message = {
		"no error",
		"cannot open resource",
		"unknown file format",
		"broken file",
		"invalid FreeType version", 
		"module version is too low", 
		"invalid argument",
		"unimplemented feature", 
		"broken table",
		"broken offset within table", 
		"array allocation size too large", 
		"missing module",
		"missing property"	
	};
	return message[code].c_str();
}

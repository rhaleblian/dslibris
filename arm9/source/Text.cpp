#include <nds.h>
#include <fat.h>
#include "Text.h"
#include "App.h"
#include "splash.h"
#include "main.h"
#include "version.h"

// TODO move this to an image service class

int getSize(uint8 *source, uint16 *dest, uint32 arg) {
       return *(uint32*)source;
}
uint8 readByte(uint8 *source) { return *source; }

void drawstack(u16 *screen) {
       TDecompressionStream decomp = {getSize, NULL, readByte};
       swiDecompressLZSSVram((void*)splashBitmap, screen, 0, &decomp);
}

Text::Text()
{
	bold = false;
	bgcolor.r = 31;
	bgcolor.g = 31;
	bgcolor.b = 15;
	usebgcolor = false;
	codeprev = 0;
	filenames[TEXT_STYLE_NORMAL] = FONTFILEPATH;
	filenames[TEXT_STYLE_BOLD] = FONTBOLDFILEPATH;
	filenames[TEXT_STYLE_ITALIC] = FONTITALICFILEPATH;
	filenames[TEXT_STYLE_BROWSER] = FONTBROWSERFILEPATH;
	filenames[TEXT_STYLE_SPLASH] = FONTSPLASHFILEPATH;
	ftc = false;
	invert = false;
	italic = false;
	justify = false;
	linebegan = false;
	linespacing = 0;
	pixelsize = PIXELSIZE;
	screenleft = (u16*)BG_BMP_RAM_SUB(0);
	screenright = (u16*)BG_BMP_RAM(0);
	screen = screenleft;
	margin.left = MARGINLEFT;
	margin.right = MARGINRIGHT;
	margin.top = MARGINTOP;
	margin.bottom = MARGINBOTTOM;
	display.height = PAGE_HEIGHT;
	display.width = PAGE_WIDTH;

	stats_hits = 0;
	stats_misses = 0;
}

Text::~Text()
{
	ClearCache();
		   
	for(map<FT_Face, Cache*>::iterator iter = textCache.begin();
		iter != textCache.end(); iter++) {
		delete iter->second;
	}

	textCache.clear();
	
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

int Text::InitWithCacheManager(void) {
	int error = FT_Init_FreeType(&library);
	if(error) return error;

	FTC_Manager_New(library,0,0,0,
		&TextFaceRequester,NULL,&cache.manager);
	FTC_ImageCache_New(cache.manager,&cache.image);
	FTC_SBitCache_New(cache.manager,&cache.sbit);
	FTC_CMapCache_New(cache.manager,&cache.cmap);

	face_id.file_path = filenames[TEXT_STYLE_NORMAL].c_str();
	face_id.face_index = 0;
	error =	FTC_Manager_LookupFace(cache.manager, (FTC_FaceID)&face_id, &faces[TEXT_STYLE_NORMAL]);
	if(error) return error;
	FT_Select_Charmap(GetFace(TEXT_STYLE_NORMAL), FT_ENCODING_UNICODE);
	charmap_index = FT_Get_Charmap_Index(GetFace(TEXT_STYLE_NORMAL)->charmap);
	imagetype.face_id = (FTC_FaceID)&face_id;
	imagetype.height = pixelsize;
	imagetype.width = pixelsize;
	ftc = true;

	return 0;
}

int Text::InitDefault(void) {
	if (FT_Init_FreeType(&library))
		return 1;
	
	map<u8, string>::iterator iter;   
	for (iter = filenames.begin(); iter != filenames.end(); iter++) {
		FT_Face face;
		
		if (FT_New_Face(library, iter->second.c_str(), 0, &face)) {
			// Failed; attempt to use the NORMAL style
			map<u8, FT_Face>::iterator find = faces.find(TEXT_STYLE_NORMAL);
			
			if (find == faces.end())
				return 2;
			else
				face = find->second;
		}
		
		char msg[MAXPATHLEN];
		strcpy(msg, "");
		sprintf(msg,"info : font '%s'\n", iter->second.c_str());
		app->Log(msg);
		
		FT_Select_Charmap(face, FT_ENCODING_UNICODE);
		FT_Set_Pixel_Sizes(face, 0, pixelsize);
		
		textCache.insert(make_pair(face, new Cache()));
		faces[iter->first] = face;
	}
	
	screen = screenleft;
	ClearCache();
	InitPen();
	ftc = false;
	return 0;
}

int Text::Init()
{
	if(ftc) return InitWithCacheManager();
	else return InitDefault();
}

void Text::Begin()
{
	bold = false;
	italic = false;
	linebegan = false;
}

void Text::End() {}

int Text::CacheGlyph(u32 ucs)
{
	return CacheGlyph(ucs, TEXT_STYLE_NORMAL);
}

int Text::CacheGlyph(u32 ucs, u8 style)
{
	return CacheGlyph(ucs, GetFace(style));
}

int Text::CacheGlyph(u32 ucs, FT_Face face)
{
	// Cache glyph at ucs if there's space.
	// Does not check if this is a duplicate entry.

	if(textCache[face]->cacheMap.size() == CACHESIZE) return -1;

	FT_Load_Char(face, ucs,
		FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL);
	FT_GlyphSlot src = face->glyph;
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
	if(!ftc) return ucs;
	return FTC_CMapCache_Lookup(cache.cmap,(FTC_FaceID)&face_id,
		charmap_index,ucs);
}

int Text::GetGlyphBitmap(u32 ucs, FTC_SBit *sbit)
{
	imagetype.flags = FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL;
	error = FTC_SBitCache_Lookup(cache.sbit,&imagetype,
		GetGlyphIndex(ucs),sbit,NULL);
	if(error) return error;
	return 0;
}

FT_GlyphSlot Text::GetGlyph(u32 ucs, int flags)
{
	return GetGlyph(ucs, flags, TEXT_STYLE_NORMAL);
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

	map<u16,FT_GlyphSlot>::iterator iter = textCache[face]->cacheMap.find(ucs);
	
	if (iter != textCache[face]->cacheMap.end()) {
		//stats_hits++;
		return iter->second;
	}
	
	//stats_misses++;
	int i = CacheGlyph(ucs, face);
	if (i > -1)
		return textCache[face]->cacheMap[ucs];

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
	if(invert) memset((void*)screen,0,PAGE_WIDTH*PAGE_HEIGHT*4);
	else memset((void*)screen,255,PAGE_WIDTH*PAGE_HEIGHT*4);
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

u8 Text::GetStringWidth(const char *txt, u8 style)
{
	return GetStringWidth(txt, GetFace(style));
}

u8 Text::GetStringWidth(const char *txt, FT_Face face)
{
	u8 width = 0;
	const char *c;
	for(c = txt; c != NULL; c++)
	{
		u32 ucs;
		GetCharCode(c, &ucs);
		width += GetAdvance(ucs, face);
	}
	return width;
}


u8 Text::GetCharCode(const char *utf8, u32 *ucs) {
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
	return (GetFace(TEXT_STYLE_NORMAL)->size->metrics.height >> 6);
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
	return GetAdvance(ucs, TEXT_STYLE_NORMAL);
}

u8 Text::GetAdvance(u32 ucs, u8 style) {
	return GetAdvance(ucs, GetFace(style));
}

u8 Text::GetAdvance(u32 ucs, FT_Face face) {
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

void Text::InitPen(void) {
	pen.x = margin.left;
	pen.y = margin.top + GetHeight();
}

void Text::PrintChar(u32 ucs)
{
	PrintChar(ucs, TEXT_STYLE_NORMAL);
}

void Text::PrintChar(u32 ucs, u8 style) {
	PrintChar(ucs, GetFace(style));
}

void Text::PrintChar(u32 ucs, FT_Face face) {
	// Draw a character for the given UCS codepoint,
	// into the current screen buffer at the current pen position.

	u16 bx, by, width, height = 0;
	FT_Byte *buffer = NULL;
	FT_UInt advance = 0;

	// get metrics and glyph pointer.

	if(ftc)
	{
		// use the FT cache.
		error = GetGlyphBitmap(ucs,&sbit);
		buffer = sbit->buffer;
		bx = sbit->left;
		by = sbit->top;
		height = sbit->height;
		width = sbit->width;
		advance = sbit->xadvance;
	}
	else
	{
		// Consult the cache for glyph data and cache it on a miss
		// if space is available.
		FT_GlyphSlot glyph = GetGlyph(ucs, 
			FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL, face);
		FT_Bitmap bitmap = glyph->bitmap;
		bx = glyph->bitmap_left;
		by = glyph->bitmap_top;
		width = bitmap.width;
		height = bitmap.rows;
		advance = glyph->advance.x >> 6;
		buffer = bitmap.buffer;
	}

	// kern.
#ifdef EXPERIMENTAL_KERNING
	if(codeprev) {
		FT_Vector k;
		error = FT_Get_Kerning(face,codeprev,ucs,FT_KERNING_UNSCALED,&k);
		if(error) {
			app->Log("warn : kerning lookup error ");
			app->Log((char *)&codeprev);
			app->Log("->");
			app->Log((char *)&ucs);
			app->Log("\n");
		}
		else if (k.x)
		{
			app->Log("info : kern ");
			app->Log((int)k.x);
			app->Log(" ");
			app->Log((char *)&codeprev);
			app->Log((char *)&ucs);
			app->Log("\n");
			pen.x += k.x >> 6;
		}
	}
#endif

	// render to framebuffer.

	u16 gx, gy;
	for (gy=0; gy<height; gy++) {
		for (gx=0; gx<width; gx++) {
			u8 a = buffer[gy*width+gx];
			if (a) {
				u16 sx = (pen.x+gx+bx);
				u16 sy = (pen.y+gy-by);
				if(usebgcolor) {
					u32 r,g,b;
					u8 alpha = 255-a;
					r = (bgcolor.r * alpha);
					g = (bgcolor.g * alpha);
					b = (bgcolor.b * alpha);
					screen[sy*display.height+sx]
						= RGB15(r/256,g/256,b/256) | BIT(15);
				} else {
					u8 l;
					if (invert) l = a >> 3;
					else l = (255-a) >> 3;
					screen[sy*display.height+sx] = RGB15(l,l,l) | BIT(15);
				}
			}
		}
	}
	pen.x += advance;
	codeprev = ucs;
}

bool Text::PrintNewLine(void) {
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
	PrintString(s, TEXT_STYLE_BROWSER);
}

void Text::PrintString(const char *s, u8 style) {
	PrintString(s, GetFace(style));
}

void Text::PrintString(const char *s, FT_Face face) {
	// draw a character string starting at the pen position.
	u32 clast = 0;
	u8 i=0;
	while(i<strlen((char*)s)) {
		u32 c = s[i];
		if (c == '\n') {
			PrintNewLine();
			i++;
		} else {
			i+=GetCharCode(&(s[i]),&c);
			PrintChar(c, face);
			clast = c;
		}
	}
}

void Text::PrintStats() {
	char msg[128];
	sprintf(msg, "info: %d cache hits.\n", stats_hits);
	app->Log(msg);
	sprintf(msg, "info: %d cache misses.\n", stats_misses);
	app->Log(msg);
}

void Text::PrintStatusMessage(const char *msg)
{
	u16 x,y;
	GetPen(&x,&y);
	u16 *s = screen;
	int ps = GetPixelSize();
	bool invert = GetInvert();
	
	screen = screenleft;
	SetInvert(false);
	SetPixelSize(8);
	SetPen(10,16);
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
	char msg[16];
	sprintf(msg,"%s",VERSION);
	PrintStatusMessage(msg);
	
	SetPixelSize(size);
	SetInvert(invert);
	SetScreen(s);
	
	swiWaitForVBlank();
}

void Text::SetFontFile(const char *filename, u8 style)
{
	filenames[style] = filename;
}

string Text::GetFontFile(u8 style)
{
	return filenames[style];
}

bool Text::SetFace(u8 style)
{
	// ha!
	return false;
}

FT_Face Text::GetFace(u8 style)
{
	map<u8, FT_Face>::iterator iter = faces.find(style);
	if (iter != faces.end())
		return iter->second;
	else
		return faces[TEXT_STYLE_NORMAL];
}


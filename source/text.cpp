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

#include "text.h"

#include <iostream>
#include <sstream>
#include <sys/param.h>

#include "nds.h"
#include "fat.h"
#include "string.h"

#include "app.h"
#include "main.h"
#include "version.h"

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
	filenames[TEXT_STYLE_BOLDITALIC] = FONTBOLDITALICFILE;
	filenames[TEXT_STYLE_BROWSER] = FONTBROWSERFILE;
	filenames[TEXT_STYLE_SPLASH] = FONTSPLASHFILE;
	screenleft = (u16*)BG_BMP_RAM_SUB(0);
	screenright = (u16*)BG_BMP_RAM(0);
	offscreen = new u16[display.width * display.height];
	margin.left = MARGINLEFT;
	margin.right = MARGINRIGHT;
	margin.top = MARGINTOP;
	margin.bottom = MARGINBOTTOM;
	bgcolor.r = 15;
	bgcolor.g = 15;
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

	ClearScreen(offscreen, 255, 255, 255);
	ss.clear();
}

Text::~Text()
{
	// framebuffers
	free(offscreen);
	
	// homemade cache
	ClearCache();
	for(std::map<FT_Face, Cache*>::iterator iter = textCache.begin();
		iter != textCache.end(); iter++) {
		delete iter->second;
	}
	textCache.clear();
	
	// FreeType
	for (std::map<u8, FT_Face>::iterator iter = faces.begin(); iter != faces.end(); iter++) {
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

	sprintf(face_id.file_path, "%s/%s", app->fontdir.c_str(), filenames[TEXT_STYLE_REGULAR].c_str());
	face_id.face_index = 0;
	error =	FTC_Manager_LookupFace(cache.manager, (FTC_FaceID)&face_id, &faces[TEXT_STYLE_REGULAR]);
	if(error) return error;

	initialized = true;
	return 0;
}

FT_Error Text::CreateFace(int style) {
	// TODO check for leakage
	std::string path = app->fontdir + "/" + filenames[style];
	error = FT_New_Face(library, path.c_str(), 0, &face);
	if (!error) {
		FT_Select_Charmap(face, FT_ENCODING_UNICODE);
		faces[style] = face;
	}
	return error;
}

int Text::InitHomemadeCache(void) {
	//! Use our own cheesey glyph cache.
	FT_Error err;

	err = FT_Init_FreeType(&library);
	if (err) return err;
	
	err = CreateFace(TEXT_STYLE_BROWSER);
	err = CreateFace(TEXT_STYLE_SPLASH);
	err = CreateFace(TEXT_STYLE_REGULAR);

	err = CreateFace(TEXT_STYLE_ITALIC);
	if (err)
		faces[TEXT_STYLE_ITALIC] = faces[TEXT_STYLE_REGULAR];

	err = CreateFace(TEXT_STYLE_BOLD);
	if (err)
		faces[TEXT_STYLE_BOLD] = faces[TEXT_STYLE_REGULAR];

	err = CreateFace(TEXT_STYLE_BOLDITALIC);
	if (err)
		faces[TEXT_STYLE_BOLDITALIC] = faces[TEXT_STYLE_REGULAR];
	
	std::map<u8, FT_Face>::iterator iter;
	for (iter = faces.begin(); iter != faces.end(); iter++) {
		FT_Set_Pixel_Sizes(iter->second, 0, pixelsize);
		textCache.insert(std::make_pair(iter->second, new Cache()));
	}

	initialized = true;
	return 0;
}

int Text::Init()
{
	if(ftc)
		return InitFreeTypeCache();
	else
		return InitHomemadeCache();
}

void Text::ReportFace(FT_Face face)
{
	printf("%s\n", face->family_name);
	printf("%s\n", face->style_name);
	printf("faces %ld\n", face->num_faces);
	printf("glyphs %ld\n", face->num_glyphs);
	printf("fixed-sizes %d\n", face->num_fixed_sizes);
	for (int i=0;i<face->num_fixed_sizes;i++)
	{
		printf(" w %d h %d\n",
			face->available_sizes[i].width,
			face->available_sizes[i].height);
	}	
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
	return CacheGlyph(ucs, style);
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

	FT_Load_Char(face, ucs,
		FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL);
	FT_GlyphSlot src = face->glyph;
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
	textCache[face]->cacheMap.insert(std::make_pair(ucs, dst));
	return ucs;
}

FT_UInt Text::GetGlyphIndex(u32 ucs)
{
	//! Given a UCS codepoint, return where it is in the charmap, by index.
	if (ftc)
		return FTC_CMapCache_Lookup(cache.cmap,
			(FTC_FaceID)&face_id, -1, ucs);
	else
		return FT_Get_Char_Index(faces[style], ucs);	
}

int Text::GetGlyphBitmap(u32 ucs, FTC_SBit *sbit, FTC_Node *anode)
{
	//! Given a UCS code, fills sbit and anode.
	//! Returns nonzero on error.
	if (!ftc) return 1;
	imagetype.face_id = (FTC_FaceID)&face_id;
	imagetype.height = pixelsize;
	imagetype.flags = FT_LOAD_RENDER;
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

	std::map<u16,FT_GlyphSlot>::iterator iter = textCache[face]->cacheMap.find(ucs);
	if (iter != textCache[face]->cacheMap.end()) {
		stats_hits++;
		hit = true;
		return iter->second;
	}
	
	hit = false;
	stats_misses++;

	int i = CacheGlyph(ucs, face);
	if (i >= 0)
		return textCache[face]->cacheMap[ucs];

	FT_Load_Char(face, ucs, flags);
	return face->glyph;
}

void Text::ClearCache()
{
	 for (std::map<u8, FT_Face>::iterator iter = faces.begin(); iter != faces.end(); iter++) {
		 ClearCache(iter->second);
	 }
}

void Text::ClearCache(u8 style)
{
	ClearCache(GetFace(style));
}

void Text::ClearCache(FT_Face face)
{
	for(std::map<u16, FT_GlyphSlot>::iterator iter = textCache[face]->cacheMap.begin();
		iter != textCache[face]->cacheMap.end();
		iter++) {
		delete iter->second;
	}
	textCache[face]->cacheMap.clear();
}

void Text::ClearScreen()
{
	const int pixelcount = display.width*display.height;
	if(invert) memset((void*)screen,0,pixelcount*4);
	else {
		memset((void*)screen,255,pixelcount*4);
		
		// alternately, off-white...
		// const u16 pixel = RGB15(29,29,29)|BIT(15);
		// for (int i=0; i<pixelcount; i++) screen[i] = pixel;
	}
}

void Text::ClearRect(u16 xl, u16 yl, u16 xh, u16 yh)
{
	u16 clearcolor;
	if(invert) clearcolor = RGB15(0,0,0) | BIT(15);
	else clearcolor = RGB15(31,31,31) | BIT(15);
	for(u16 y=yl; y<yh; y++) {
		for(u16 x=xl; x<xh; x++) {
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
	//! Return total advance in pixels.
	u8 width = 0;
	const char *c;
	for(c = txt; c != NULL; c++)
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

std::string Text::GetFontName(u8 style) {
	return
		std::string(faces[style]->family_name)
		+ " "
		+ std::string(faces[style]->style_name);
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
	if (size == pixelsize) return;
	pixelsize = size;
	if (ftc)
	{
		imagetype.height = pixelsize;
		imagetype.width = pixelsize;
		pixelsize = pixelsize;
	}
	else
	{
		for (auto iter = faces.begin(); iter != faces.end(); iter++)
		{
			FT_Set_Pixel_Sizes(iter->second, 0, pixelsize);
			ClearCache(face);
		}
	}
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
	

	if (ftc) {
		imagetype.flags = FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP;

		// error = FTC_SBitCache_Lookup(cache.sbit,&imagetype,
		// 	GetGlyphIndex(ucs),&sbit,NULL);
		// return sbit->xadvance;

		FT_Glyph glyph;
		FTC_ImageType type = &imagetype;
		error = FTC_ImageCache_Lookup(cache.image,type,GetGlyphIndex(ucs),&glyph,NULL);

		return (glyph->advance).x;
	}
	else
	{
		// Also caches this glyph.
		return GetGlyph(ucs, FT_LOAD_DEFAULT, face)->advance.x >> 6;

		// auto gindex = FT_Get_Char_Index(face, ucs);
		// FT_Load_Glyph(face, gindex, FT_LOAD_DEFAULT);
		// return face->glyph->advance.x >> 16;
	}
}

int Text::GetStringAdvance(const char *s) {
	int advance = 0;
	for(unsigned int i=0;i<strlen(s);i++) {
		advance += GetAdvance(s[i]);
	}
	return advance;
}

bool Text::GetFontName(std::string &s) {
	const char *name = FT_Get_Postscript_Name(GetFace(style));
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

void Text::PrintChar(u32 ucs)
{
	PrintChar(ucs, GetFace(style));
}

void Text::PrintChar(u32 ucs, u8 astyle) {
	PrintChar(ucs, GetFace(astyle));
}

void Text::PrintChar(u32 ucs, FT_Face face) {
	// Draw a character for the given UCS codepoint,
	// into the current screen buffer at the current pen position.

	u16 bx, by, width, height = 0;
	FT_Byte *buffer = NULL;
	FT_UInt advance = 0;
	FTC_Node anode = nullptr;
	FT_Glyph glyph;

	if(ftc)
	{
	    auto glyph_index = FTC_CMapCache_Lookup(cache.cmap, (FTC_FaceID)&face_id, -1, ucs);
		// TODO set imagetype here

		FTC_SBit p = &sbit;
  		error = FTC_SBitCache_Lookup(cache.sbit,
                            	&imagetype,
								glyph_index,
                                &p,
                                &anode );
		if (error) return;

		// there will typically be no bitmap, only an outline
		if (!p)
		{
			error = FTC_ImageCache_Lookup(cache.image, &imagetype, glyph_index, &glyph, &anode);
			if (error) return;
		}
		// TODO rasterize bitmap

		buffer = sbit.buffer;
		bx = sbit.left;
		by = sbit.top;
		height = sbit.height;
		width = sbit.width;
		advance = sbit.xadvance;
	}
	else
	{
		// Consult the cache for glyph data and cache it on a miss, if space is available.
		FT_GlyphSlot glyph = GetGlyph(ucs, FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL, face);

  		// extract glyph image
		FT_Bitmap bitmap = glyph->bitmap;
		bx = glyph->bitmap_left;
		by = glyph->bitmap_top;
		width = bitmap.width;
		height = bitmap.rows;
		advance = glyph->advance.x >> 6;
		buffer = bitmap.buffer;
	}

#ifdef EXPERIMENTAL_KERNING
	// Fetch a kerning vector.
	if(codeprev) {
		FT_Vector kerning_vector;
		error = FT_Get_Kerning(face, codeprev, ucs, FT_KERNING_DEFAULT, &kerning_vector);
	}
#endif

	// Render to framebuffer.

#ifdef DRAW_PEN_POSITION
	// Mark the pen position.
	screen[pen.y*display.height+pen.x] = RGB15(0, 0, 0) | BIT(15);
#endif

	u16 gx, gy;
	for (gy=0; gy<height; gy++)
	{
		for (gx=0; gx<width; gx++)
		{
			u8 a = buffer[gy*width+gx];
			if (!a) continue;
			if (!invert) a = 256 - a;
			u16 pixel = RGB15(a>>3,a>>3,a>>3)|BIT(15);
#ifdef DRAW_CACHE_MISSES
			// if(!hit) pixel = RGB15(a>>3,0,0) | BIT(15);
#endif
			u16 sx = (pen.x+gx+bx);
			u16 sy = (pen.y+gy-by);
			screen[sy*display.height+sx] = pixel;
		}
	}

	pen.x += advance;
	codeprev = ucs;

	if (ftc && anode)
		FTC_Node_Unref(anode, cache.manager);
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
	PrintString(s, style);
}

void Text::PrintString(const char *s, u8 style) {
	//! Render a character string starting at the pen position.
	PrintString(s, GetFace(style));
}

void Text::PrintString(const char *s, FT_Face face) {
	//! Render a character string starting at the pen position.
	u8 i=0;
	while(i<strlen((char*)s)) {
		u32 c = s[i];
		if (c == '\n') {
			PrintNewLine();
			i++;
		} else {
			i+=GetCharCode(&(s[i]),&c);
			PrintChar(c, face);
		}
	}
}

void Text::ClearScreen(u16 *screen, u8 r, u8 g, u8 b)
{
	for (int i=0;i<display.height*display.height;i++)
		screen[i] = RGB15(r,g,b) | BIT(15);
}

void Text::PrintSplash(u16 *screen)
{
	// push
	auto s = GetScreen();
	auto z = GetPixelSize();
	auto i = GetInvert();

	SetScreen(screen);
	drawstack(screen);
	app->PrintStatus(VERSION);

	// pop
	SetInvert(i);
	SetPixelSize(z);
	SetScreen(s);
}

void Text::SetFontFile(const char *path, u8 style)
{
	if (!strcmp(filenames[style].c_str(), path)) return;
	filenames[style] = std::string(path);
	CreateFace(style);
}

std::string Text::GetFontFile(u8 style)
{
	return filenames[style];
}

bool Text::SetFace(u8 astyle)
{
	style = astyle;
	face = faces[style];
	return true;
}

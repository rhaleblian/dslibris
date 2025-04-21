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

#include <nds.h>
#include "fat.h"
#include "string.h"

#include "app.h"
#include "main.h"
#include "version.h"
#include "text.h"

Text::Text()
{
	filenames[TEXT_STYLE_REGULAR] = FONTREGULARFILE;
	filenames[TEXT_STYLE_BOLD] = FONTBOLDFILE;
	filenames[TEXT_STYLE_ITALIC] = FONTITALICFILE;
	filenames[TEXT_STYLE_BROWSER] = FONTBROWSERFILE;
	filenames[TEXT_STYLE_SPLASH] = FONTSPLASHFILE;
	
	margin.left = MARGINLEFT;
	margin.right = MARGINRIGHT;
	margin.top = MARGINTOP;
	margin.bottom = MARGINBOTTOM;
	bgcolor.r = 31;
	bgcolor.g = 31;
	bgcolor.b = 15;
	justify = false;
	linespacing = 0;
	usebgcolor = false;

	imagetype.face_id = (FTC_FaceID)&face_id;
	imagetype.flags = FT_LOAD_DEFAULT; 
	imagetype.height = pixelsize;
	imagetype.width = 0;

	display.height = PAGE_HEIGHT;
	display.width = PAGE_WIDTH;
	ftc = false;

	// Rendering state.
	bold = false;
	codeprev = 0;
	invert = false;
	hit = false;
	italic = false;
	face = nullptr;
	linebegan = false;
	style = TEXT_STYLE_REGULAR;
	pixelsize = PIXELSIZE;
	screen = screenleft = screenright = nullptr;	
	InitPen();

	// Statistics.
	stats_hits = 0;
	stats_misses = 0;

	initialized = false;
}

Text::~Text()
{
	// homemade cache
	// ClearCache();
	// for(map<FT_Face, Cache*>::iterator iter = textCache.begin();
	// 	iter != textCache.end(); iter++) {
	// 	delete iter->second;
	// }
	// textCache.clear();
	
	// FreeType
	map<u8, FT_Face>::iterator iter;
	for (iter = faces.begin(); iter != faces.end(); iter++) {
		FT_Done_Face(iter->second);
	}
	FT_Done_FreeType(library);
}

void Text::CopyScreen(u16 *src, u16 *dst) {
	memcpy(src, dst, display.width * display.height * sizeof(u16));
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
	app->Log(msg);
	error =	FTC_Manager_LookupFace(cache.manager, (FTC_FaceID)&face_id, &faces[TEXT_STYLE_REGULAR]);
	if(error) return error;
	app->Log("ok\n");
	// FT_EXPORT( FT_Error )
//   FTC_Manager_LookupSize( FTC_Manager  manager,
//                           FTC_Scaler   scaler,
//                           FT_Size     *asize );

	app->Log("initialized freetype cache\n");
	return 0;
}

FT_Error Text::CreateFace(int style) {
	std::string path = std::string(FONTDIR) + "/" + filenames[style];
	FT_Face f = faces[style];

	FT_Error error = FT_New_Face(library, path.c_str(), 0, &f);
	if (error) {
		app->Log(FT_Error_String(error));
		return error;
	}
	
	error = FT_Select_Charmap(f, FT_ENCODING_UNICODE);
	if (error)
		app->Log(FT_Error_String(error));

	FT_Set_Pixel_Sizes(f, 0, pixelsize);
	if (error)
		app->Log(FT_Error_String(error));

	return error;
}

int Text::InitHomemadeCache(void) {
	//! Use our own cheesey glyph cache.
	
	std::map<u8, FT_Face>::iterator iter;
	for (iter = faces.begin(); iter != faces.end(); iter++)
		textCache.insert(make_pair(iter->second, new Cache()));

	ClearCache();
	app->Log("custom cache initialized\n");
	return 0;
}

int Text::Init()
{
	error = FT_Init_FreeType(&library);
	if (error) return error;

	error = CreateFace(TEXT_STYLE_REGULAR);
	error |= CreateFace(TEXT_STYLE_ITALIC);
	error |= CreateFace(TEXT_STYLE_BOLD);
	error |= CreateFace(TEXT_STYLE_BROWSER);
	error |= CreateFace(TEXT_STYLE_SPLASH);
	if (error) return error;

	// ReportFace(faces[TEXT_STYLE_REGULAR]);
	
	// FT_Set_Pixel_Sizes(faces[TEXT_STYLE_REGULAR], 0, pixelsize);
	// FT_Set_Pixel_Sizes(faces[TEXT_STYLE_ITALIC], 0, pixelsize);
	// FT_Set_Pixel_Sizes(faces[TEXT_STYLE_BOLD], 0, pixelsize);
	// FT_Set_Pixel_Sizes(faces[TEXT_STYLE_BROWSER], 0, pixelsize);
	// FT_Set_Pixel_Sizes(faces[TEXT_STYLE_SPLASH], 0, pixelsize);

	initialized = true;
	return error;
}

const char* Text::GetError(FT_Error error_code) {
	return FT_Error_String(error_code);
}

void Text::ReportFace(FT_Face face)
{
	sprintf(msg, "family_name %s\n", face->family_name);
	app->Log(msg);
	sprintf(msg, "style_name %s\n", face->style_name);
	app->Log(msg);
	sprintf(msg, "num_faces %ld\n", face->num_faces);
	app->Log(msg);
	sprintf(msg, "num_glyphs %ld\n", face->num_glyphs);
	app->Log(msg);
	sprintf(msg, "num_fixed_sizes %d\n", face->num_fixed_sizes);
	app->Log(msg);
	for (int i=0;i<face->num_fixed_sizes;i++)
	{
		sprintf(msg, "size %d x %d\n",
			face->available_sizes[i].width,
			face->available_sizes[i].height);
		app->Log(msg);
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

	map<u16,FT_GlyphSlot>::iterator iter = textCache[face]->cacheMap.find(ucs);
	
	if (iter != textCache[face]->cacheMap.end()) {
		stats_hits++;
		hit = true;
		return iter->second;
	}
	
	stats_misses++;
	hit = false;
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
	u16 color;
	if (invert)
		color = ARGB16(1, 0, 0, 0);
	else
		color = ARGB16(1, 31, 31, 31);
	//u word = (clearcolor << 16) | clearcolor;
	for(u16 y=yl; y<yh; y++) {
//		memcpy((void*)screen[y*display.height+xl],(void*)word,xh-xl/2);
		for(u16 x=xl; x<xh; x++) {
			screen[y*display.height+x] = color;
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

u8 Text::GetCharCountInsideWidth(const char *txt, u8 pixels) {
	u8 n = 0;
	u8 width = 0;
	const char *c;
	for(c = txt; c != NULL; c++)
	{
		u32 ucs = 0;
		GetCharCode(c, &ucs);
		width += GetAdvance(ucs, GetFace());
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
	// if (!face) {
	// 	app->Log("face is null\n");
	// 	return;
	// }
	
	// error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	// if (error)
	// 	app->Log(FT_Error_String(error));
	// else
	// 	app->Log("selected unicode charmap\n");

	FT_UInt glyph_index = FT_Get_Char_Index(face, ucs);
	sprintf(msg, "will load glyph index %d\n", glyph_index);
	app->Log(msg);

	error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
	if (error)
		app->Log(FT_Error_String(error));
	else
		app->Log("glyph loaded\n");

	u16 bx = face->glyph->bitmap_left;
	u16 by = face->glyph->bitmap_top;
	sprintf(msg, "%d %d\n", bx, by); app->Log(msg);

	// error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
	// if (error) {
	// 	sprintf(msg, "%s\n", FT_Error_String(error)); app->Log(msg);
	// } else
	// 	app->Log("rendered glyph to slot\n");

	u16 width = face->glyph->bitmap.width;
	u16 height = face->glyph->bitmap.rows;
	FT_UInt advance = face->glyph->advance.x >> 6;
	sprintf(msg, "info : index %d is %u x %u advancing %d\n",
		face->glyph->glyph_index, width, height, advance); app->Log(msg);

	FT_Byte* buffer = face->glyph->bitmap.buffer;
	if (!buffer) {
		app->Log("warn : glyph buffer is null\n");
	}

	// render glyph to framebuffer.

	u8 a = 0;
	for (u16 gy=0; gy<height; gy++) {
		for (u16 gx=0; gx<width; gx++) {
			if (buffer) a = buffer[gy*width+gx];
			u16 sx = (pen.x+gx+bx);
			u16 sy = (pen.y+gy-by);
			u8 l = a >> 3;
			screen[sy*LCD_WIDTH+sx] = ARGB16(1, l, l, l);
		}
	}

	pen.x += advance;
	codeprev = ucs;
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
	//app->Log(s);
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
	app->Log(msg);
	sprintf(msg, "info: %d cache misses.\n", stats_misses);
	app->Log(msg);
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

void Text::PrintSplash()
{
	char msg[BUFSIZE];
	u16* s = GetScreen();
	bool i = GetInvert();
	u8 size = GetPixelSize();

	sprintf(msg, "%s", VERSION);
	SetScreen(screenleft);
	drawstack(screen);
	PrintStatusMessage(msg);
	
	SetPixelSize(size);
	SetInvert(i);
	SetScreen(s);
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

typedef struct {
	char *path;
	u32 faceindex;
  } Designator;
  
  FT_Error requester(FTC_FaceID face_id, FT_Library library, FT_Pointer req_data,
					 FT_Face *aface) {
	auto designator = (Designator *)req_data;
	return FT_New_Face(library, designator->path, 0, aface);
  }
  
  FT_Freeables typesetter() {
	FT_Freeables f;
	f.face = nullptr;
	f.library = nullptr;
	f.manager = nullptr;
	// char filepath[128] =
	//     "/home/ray/GitHub/dslibris/cflash/font/LiberationSerif-Regular.ttf";
	char filepath[128] = "/Font/regular.ttf";
	const u32 char_code = 38; // ASCII &
	Designator designator;
	designator.path = filepath;
	designator.faceindex = 0;
  
	FTC_FaceID face_id = &designator;
	FT_Library library;
	FT_Face face;
	FTC_Manager manager;
	// FTC_ScalerRec scaler;
	FTC_ImageCache cache;
	FTC_ImageTypeRec imagetyperec;
	FTC_ImageType imagetype = &imagetyperec;
	FT_UInt glyph_index = 0;
	FT_Glyph glyph;
	FTC_Node node;
	FTC_CMapCache cmcache;
	// char msg[256];
  
	FT_Error error = FT_Init_FreeType(&library);
	if (error) return f;
	error =
		FTC_Manager_New(library, 1, 1, 1000000, requester, &designator, &manager);
	error = FTC_ImageCache_New(manager, &cache);
	error = FTC_CMapCache_New(manager, &cmcache);
  
	// Get the face.
  
	//  error = FT_New_Face(library, designator.path, 0, &face);
	error = FTC_Manager_LookupFace(manager, face_id, &face);
  
	// Set a size.
  
  #if 0  
	FT_Size_RequestRec rec;
	FT_Request_Size(face, &rec);
	error = FT_Set_Char_Size(face,    /* handle to face object           */
							 0,       /* char_width in 1/64th of points  */
							 16 * 64, /* char_height in 1/64th of points */
							 300,     /* horizontal device resolution    */
							 300);
	or
	error = FT_Set_Pixel_Sizes(
		  face,   /* handle to face object */
		  0,      /* pixel_width           */
		  16 );   /* pixel_height          */
  
	Get the glyph index.
  
	char_code = FT_Get_First_Char(face, &glyph_index);
	glyph_index = FT_Get_Char_Index(face, char_code);
  #endif
	glyph_index = FTC_CMapCache_Lookup(cmcache, face_id, 0, char_code);
  
	// Get the glyph fron the glyph index.
  
	// error = FT_Load_Glyph(face,                     /* handle to face object */
	//                       glyph_index,              /* glyph index           */
	//                       FT_LOAD_DEFAULT);         /* load flags, see below */
	imagetype->face_id = face_id;
	imagetype->flags = FT_LOAD_DEFAULT;
	imagetype->height = 16;
	imagetype->width = 0;
	error = FTC_ImageCache_Lookup(cache, imagetype, glyph_index, &glyph, &node);
  
	error = FT_Render_Glyph(face->glyph,            /* glyph slot  */
							FT_RENDER_MODE_NORMAL); /* render mode */

	f.face = face;
	f.library = library;
	f.manager = manager;
	return f;
  }
  
  #if 0
  
  #ifdef ASCIIART
  int asciiart() {
	auto ft = typesetter();
	auto error = renderer(ft.face);
	free_ft(ft);
	return error;
  }
  
  int renderer(FT_Face face) {
	std::string s;
	auto bitmap = face->glyph->bitmap;
	for (u y = 0; y < bitmap.rows; y++) {
	  s.clear();
	  for (u x = 0; x < bitmap.width; x++) {
		u v = bitmap.buffer[y * bitmap.width + x];
		if (v)
		  s.append("&");
		else
		  s.append(" ");
	  }
	  std::cerr << s << std::endl;
	  iprintf(s.c_str());
	}
	return 0;
  }
  #else
  int renderer(FT_Face face) {
	// Make something to draw things with.
  
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
	  std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
	  return 1;
	}
  
	SDL_Window *win =
		SDL_CreateWindow("FreeType Rendering", 100, 100, D, D, SDL_WINDOW_SHOWN);
	if (win == nullptr) {
	  std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
	  SDL_Quit();
	  return 1;
	}
  
	SDL_Renderer *ren = SDL_CreateRenderer(
		win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ren == nullptr) {
	  SDL_DestroyWindow(win);
	  std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
	  SDL_Quit();
	  return 1;
	}
  
	SDL_Surface *surf = SDL_CreateRGBSurface(
		0, face->glyph->bitmap.width, face->glyph->bitmap.rows, 32, 0, 0, 0, 0);
	SDL_memcpy(surf->pixels, face->glyph->bitmap.buffer,
			   face->glyph->bitmap.width * face->glyph->bitmap.rows);
	auto tex = SDL_CreateTextureFromSurface(ren, surf);
  
	// A sleepy rendering loop, wait for 3 seconds and render and present the
	// screen each time
	for (int i = 0; i < 3; ++i) {
	  // First clear the renderer
	  SDL_SetRenderDrawColor(ren, 127, 127, 127, 255);
	  SDL_RenderClear(ren);
	  // Draw the texture
	  SDL_RenderCopy(ren, tex, NULL, NULL);
	  // Update the screen
	  SDL_RenderPresent(ren);
	  // Take a quick break after all that hard work
	  SDL_Delay(1000);
	}
  
	SDL_FreeSurface(surf);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
  }
  #endif
  
  void free_ft(FT_Freeables f) {
	FTC_Manager_Reset(f.manager);
	FTC_Manager_Done(f.manager);
	// FT_Done_Face(f.face);
	FT_Done_FreeType(f.library);
  }
  
  int ft_main(int argc, char **argv) {
	auto ft = typesetter();
	auto error = renderer(ft.face);
	free_ft(ft);
	return error;
  }
  
  // int SDL_BlitSurface(SDL_Surface*    src,
  //                     const SDL_Rect* srcrect,
  //                     SDL_Surface*    dst,
  //                     SDL_Rect*       dstrect)
  
  // FTC_SBitCache_Lookup(cache.sbit,&imagetype, GetGlyphIndex(ucs),sbit,anode)
  // FT_GlyphSlot glyph = GetGlyph(ucs,
  // 	FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL, face);
  // FT_Bitmap bitmap = glyph->bitmap;
  // bx = glyph->bitmap_left;
  // by = glyph->bitmap_top;
  // width = bitmap.width;
  // height = bitmap.rows;
  // advance = glyph->advance.x >> 6;
  // buffer = bitmap.buffer;
  #endif
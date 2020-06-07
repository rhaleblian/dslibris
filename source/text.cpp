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

	ClearScreen(offscreen, 255, 255, 255);
	ss.clear();
}

Text::~Text()
{
	// framebuffers
	free(offscreen);
	
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
	app->Log("ok\n");
	error = FTC_Manager_New(library,0,0,0,
		&TextFaceRequester,NULL,&cache.manager);
	if(error) return error;
	app->Log("ok\n");
	error = FTC_ImageCache_New(cache.manager,&cache.image);
	if(error) return error;
	app->Log("ok\n");
	error = FTC_SBitCache_New(cache.manager,&cache.sbit);
	if(error) return error;
	app->Log("ok\n");
	error = FTC_CMapCache_New(cache.manager,&cache.cmap);
	if(error) return error;
	app->Log("ok\n");

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

	ReportFace(faces[TEXT_STYLE_REGULAR]);

	// FT_Select_Charmap(GetFace(TEXT_STYLE_REGULAR), FT_ENCODING_UNICODE);
	// charmap_index = FT_Get_Charmap_Index(GetFace(TEXT_STYLE_REGULAR)->charmap);

	screen = screenleft;
	InitPen();
	initialized = true;
	app->Log("initialized freetype cache\n");
	return 0;
}

FT_Error Text::CreateFace(int style) {
	std::string path = std::string(FONTDIR) + "/" + filenames[style];
	FT_Error err = FT_New_Face(library, path.c_str(), 0, &face);
	if (!err)
		faces[style] = face;
	return err;
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

	std::map<u8, FT_Face>::iterator iter;
	for (iter = faces.begin(); iter != faces.end(); iter++) {
		FT_Set_Pixel_Sizes(iter->second, 0, pixelsize);
		textCache.insert(make_pair(iter->second, new Cache()));
	}

	// map<u8, string>::iterator iter;
	// for (iter = filenames.begin(); iter != filenames.end(); iter++) {
		// std::string path = std::string("/font/") + iter->second;
		// err = FT_New_Face(library, path.c_str(), 0, &face);
		// if (err) return err;
		// // if (err) {
		// // 	// Assume this is a non-regular lookup.
		// // 	// Use the NORMAL/Regular face.
		// // 	map<u8, FT_Face>::iterator find = faces.find(TEXT_STYLE_REGULAR);			
		// // 	if (find == faces.end())
		// // 		return err;
		// // 	face = find->second;
		// // }
		
		// // FT_Select_Charmap(face, FT_ENCODING_UNICODE);
		// FT_Set_Pixel_Sizes(face, 0, pixelsize);
		
		// textCache.insert(make_pair(face, new Cache()));
		// faces[iter->first] = face;

		// sprintf(msg, "%s\n", path.c_str());
		// app->Log(msg);
		// ReportFace(face);	
	// }	

	screen = screenleft;
	ClearCache();
	InitPen();
	initialized = true;
	app->Log("custom cache initialized\n");
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
	sprintf(msg, "%s\n", face->family_name);
	app->Log(msg);
	sprintf(msg, "%s\n", face->style_name);
	app->Log(msg);
	sprintf(msg, "faces %ld\n", face->num_faces);
	app->Log(msg);
	sprintf(msg, "glyphs %ld\n", face->num_glyphs);
	app->Log(msg);
	sprintf(msg, "fixed-sizes %d\n", face->num_fixed_sizes);
	app->Log(msg);
	for (int i=0;i<face->num_fixed_sizes;i++)
	{
		sprintf(msg, " w %d h %d\n",
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

	// static bool firsttime = true;
	u16 bx, by, width, height = 0;
	FT_Byte *buffer = NULL;
	FT_UInt advance = 0;
	FTC_Node anode = nullptr;
	FT_Glyph glyph;

	ss.clear();

	// get metrics and glyph pointer.

	if(ftc)
	{
		// use the FT cache.

	    auto glyph_index = FTC_CMapCache_Lookup(cache.cmap, (FTC_FaceID)&face_id, -1, ucs);
		error = FTC_ImageCache_Lookup(cache.image, &imagetype, glyph_index, &glyph, &anode);
		if (error) {
			ss << "error " << error << std::endl;
			app->Log(ss.str().c_str());
			return;
		}
		app->Log("ok\n");

		FTC_SBit p = &sbit;
  		error = FTC_SBitCache_Lookup(cache.sbit,
                            	&imagetype,
								glyph_index,
                                &p,
                                &anode );
		if (error) {
			ss << "error " << error << std::endl;
			app->Log(ss.str().c_str());
			return;
		}
		app->Log("ok\n");

		buffer = sbit.buffer;
		bx = sbit.left;
		by = sbit.top;
		height = sbit.height;
		width = sbit.width;
		advance = sbit.xadvance;

		error = FT_Render_Glyph(faces[TEXT_STYLE_REGULAR]->glyph,            /* glyph slot  */
        	                    FT_RENDER_MODE_NORMAL); /* render mode */
		if (error) {
			ss << "error " << error << std::endl;
			app->Log(ss.str().c_str());
			return;
		}
		app->Log("ok\n");

		// auto glyph = faces[TEXT_STYLE_REGULAR]->glyph;
		// buffer = glyph->bitmap.buffer;
		// bx = glyph->bitmap_left;
		// by = glyph->bitmap_top;
		// width = glyph->bitmap.width;
		// height = glyph->bitmap.rows;
		// advance = width;

		// ss.clear();
		// ss << " err " << error 
		//    << " glyph_index " << glyph_index  << " glyph " << glyph 
		//    << " width " << width << " height " << height << " advance " << advance
		//    << std::endl;
		// app->Log(ss.str());
	}
	else
	{
		// Consult the cache for glyph data and cache it on a miss, if space is available.
		FT_GlyphSlot glyph = GetGlyph(ucs, FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL, face);
		
		// ss << "ucs " << ucs << std::endl;
		// app->Log(ss.str());

		// error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
		// if (error) app->Log("boo\n");
		// error = FT_Load_Char(face, ucs, FT_LOAD_RENDER|FT_LOAD_TARGET_REGULAR);
		// if (error) app->Log("hoo\n");
		// auto glyph = face->glyph;
  		// error = FT_Get_Glyph( face->glyph, &glyph );
		// if (error) app->Log("foo\n");

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
	// kern.

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

#ifdef DEBUG
	// DEBUG Mark the pen position.
	screen[pen.y*display.height+pen.x] = RGB15(0, 0, 0) | BIT(15);
#endif

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
#ifdef DEBUG_CACHE
					// Draw cache hits in red.
					if(!hit)
						screen[sy*display.height+sx] = RGB15(l,0,0) | BIT(15);
					else
#endif
					screen[sy*display.height+sx] = RGB15(l,l,l) | BIT(15);
				}
			}
		}
	}

	pen.x += advance;
	codeprev = ucs;

	// Release the glyph storage.
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

int asciiart() {
  auto ft = typesetter();
  auto error = renderer(ft.face);
  free_ft(ft);
  return error;
}

const char* ErrorString(uint c) {
	switch (c) {
		case 0:
		return "ok";
		break;
		default:
		return "unknown error";
	}
}

//    "no error",
//     "cannot open resource" ,
//     "unknown file format" ,
//     "broken file" ,
//     "invalid FreeType version" 
//     "module version is too low", 
//     "invalid argument" 
//     "unimplemented feature" 
//     "broken table" 
//     "broken offset within table" 
//     "array allocation size too large" 
//     "missing module" 
//     "missing property" 
// ]

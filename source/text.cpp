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

#include <fat.h>
#include <iostream>
#include <nds.h>
#include <sstream>
#include <string.h>
#include <sys/param.h>

#include "app.h"
#include "main.h"
#include "version.h"

Text::Text()
{
	display.height = PAGE_HEIGHT;
	display.width = PAGE_WIDTH;
	screenleft = (u16*)BG_BMP_RAM_SUB(0);
	screenright = (u16*)BG_BMP_RAM(0);
	screen = screenleft;
	margin.left = MARGINLEFT;
	margin.right = MARGINRIGHT;
	margin.top = MARGINTOP;
	margin.bottom = MARGINBOTTOM;
	bgcolor.r = 28;
	bgcolor.g = 28;
	bgcolor.b = 26;
	usebgcolor = false;

	style = TEXT_STYLE_BROWSER;
	pixelsize = PIXELSIZE;
	imagetype.face_id = (FTC_FaceID)&styles[style];
	imagetype.flags = FT_LOAD_DEFAULT; 
	imagetype.height = pixelsize;
	imagetype.width = 0;

	// Rendering state.
	hit = false;
	linebegan = false;
	codeprev = 0;
	bold = false;
	italic = false;
	invert = false;
	justify = false;
	linespacing = 0;

	initialized = false;
}

Text::~Text()
{
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

	error = FT_Init_FreeType(&library);
	if(error) return error;
	error = FTC_Manager_New(library,0,0,0,
		&TextFaceRequester,NULL,&cache.manager);
	error |= FTC_ImageCache_New(cache.manager,&cache.image);
	error |= FTC_SBitCache_New(cache.manager,&cache.sbit);
	error |= FTC_CMapCache_New(cache.manager,&cache.cmap);
	return error;
}

int Text::Init()
{
	error = InitFreeTypeCache();
	if (error) return error;

	SetFont(TEXT_STYLE_BOLD, "b.ttf");
	SetFont(TEXT_STYLE_BROWSER, "m.ttf");
	SetFont(TEXT_STYLE_ITALIC, "i.ttf");
	SetFont(TEXT_STYLE_REGULAR, "r.ttf");

	// check sanity

	// error =	FTC_Manager_LookupFace(cache.manager,
	// 	(FTC_FaceID)&styles[style], &face);
	// if(error) return error;
	// iprintf("[DBUG] family=%s\n", face->family_name);
	
	// FT_UInt32 codepoint = 0x46;
	// FT_UInt gindex = FTC_CMapCache_Lookup(cache.cmap,
	// 	(FTC_FaceID)&styles[style], -1, codepoint);
	// iprintf("[DBUG] gindex=%d\n", gindex);

	return error;
}

FT_Face Text::GetFace(u8 astyle) {
	FT_Face face;
	error =	FTC_Manager_LookupFace(cache.manager,
			(FTC_FaceID)&styles[astyle], &face);
	return face;
}

FT_Face Text::GetFace() {
	return GetFace(style);
}

void Text::ReportFace(FT_Face face)
{
	printf("family %s\n", face->family_name);
	printf("style %s\n", face->style_name);
	printf("num faces %ld\n", face->num_faces);
	printf("num glyphs %ld\n", face->num_glyphs);
	printf("num fixed sizes %d\n", face->num_fixed_sizes);
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
	hit = false;
}

void Text::End() {}

FT_UInt Text::GetGlyphIndex(u32 ucs)
{
	//! Given a UCS codepoint, return its gylph index.
	return FTC_CMapCache_Lookup(cache.cmap, (FTC_FaceID)&styles[style],
		-1, ucs);
}

void Text::ClearScreen()
{
	int n = display.width*display.height*4;
	if(invert) memset((void*)screen, 0, n);
	else memset((void*)screen, 255, n);
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

u8 Text::GetStringWidth(const char *txt, u8 style)
{
	return GetStringWidth(txt, GetFace(style));
}

u8 Text::GetCharCountInsideWidth(const char *str, u8 style, u8 width) {
	u8 count = 0;
	u8 x = 0;

	const char *c;
	for(c = str; c != NULL; c++)
	{
		u32 ucs = 0;
		GetCharCode(c, &ucs);
		x += GetAdvance(ucs, GetFace(style));
		if (x > width) return count;
		count++;
	}

	return count;
}

u8 Text::GetCharCode(const char *utf8, u32 *ucs) {
	//! Given a start position in UTF-8 encoding, fill in the Unicode/UCS code point.
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
	FTC_ScalerRec rec;
	rec.face_id = (FTC_FaceID)&face;
	rec.width = 0;
	rec.height = pixelsize;
	rec.pixel = 1;
	
	FT_Size size;
	error = FTC_Manager_LookupSize(cache.manager, &rec, &size);
	if (error) return 1;
	if (!size) return 1;
	return size->metrics.height >> 6;
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
	imagetype.height = size;
	imagetype.width = size;
	pixelsize = size;
}

void Text::SetScreen(u16 *inscreen)
{
	screen = inscreen;
}

u8 Text::GetAdvance(u32 ucs, FT_Face face) {
	//! Return glyph advance in pixels.
	//! All other flavours of GetAdvance() call this one.
	imagetype.flags = FT_LOAD_NO_BITMAP;
	FT_Glyph glyph;
	error = FTC_ImageCache_Lookup(cache.image, &imagetype,
		GetGlyphIndex(ucs), &glyph, NULL);
	imagetype.flags = FT_LOAD_DEFAULT;
	if (error) return 1;
	if (!glyph) return 1;
	return glyph->advance.x;
}

u8 Text::GetAdvance(u32 ucs, u8 astyle) {
	return GetAdvance(ucs, GetFace(astyle));
}

u8 Text::GetAdvance(u32 ucs) {
	return GetAdvance(ucs, GetFace(style));
}

int Text::GetStringAdvance(const char *s) {
	int advance = 0;
	for(unsigned int i=0;i<strlen(s);i++) {
		advance += GetAdvance(s[i]);
	}
	return advance;
}

bool Text::GetFontName(std::string &s) {
	const char *name = FT_Get_Postscript_Name(GetFace());
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

void Text::PrintChar(u32 charcode) {
	// Draw a character for the given UCS codepoint,
	// using the typesetter's current state.

	FTC_Node anode;
	FT_Glyph glyph;

	// Get the codepoint's glyph index.
	FT_UInt glyph_index = FTC_CMapCache_Lookup(cache.cmap,
		(FTC_FaceID)&styles[style], -1, charcode);

	// Get the glyph bitmap.
	imagetype.face_id = (FTC_FaceID)&styles[style];
	imagetype.flags = FT_LOAD_RENDER;
	imagetype.height = pixelsize;
	imagetype.width = 0;
	error = FTC_ImageCache_Lookup(cache.image,
		&imagetype, glyph_index, &glyph, &anode);

	FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
	// iprintf("%d %d %d %d %d\n", bitmap_glyph->left, bitmap_glyph->top,
	// bitmap_glyph->bitmap.rows, bitmap_glyph->bitmap.width,
	// bitmap_glyph->bitmap.pitch);
	CopyBitmap(bitmap_glyph);

	pen.x += glyph->advance.x >> 16;
	codeprev = charcode;

	// Release the glyph storage.
	if (anode)
		FTC_Node_Unref(anode, cache.manager);
}

void Text::CopyBitmap(FT_BitmapGlyph glyph) {
	//! render to framebuffer.
	FT_Bitmap bitmap = glyph->bitmap;
	if (bitmap.buffer == nullptr) return;
	u16 bx = glyph->left;
	u16 by = glyph->top;
	u16 width = bitmap.pitch;
	u16 height = bitmap.rows;

	// DEBUG Mark the pen position.
	// screen[pen.y*display.height+pen.x] = RGB15(0, 31, 0) | BIT(15);

	u16 color;
	for (int gy=0; gy<height; gy++) {
		for (int gx=0; gx<width; gx++) {
			u8 a = bitmap.buffer[gy*bitmap.pitch+gx];
			if (a) {
				u16 sx = (pen.x+gx+bx);
				u16 sy = (pen.y+gy-by);
				if(usebgcolor) {
					u32 r,g,b;
					u8 alpha = 255-a;
					r = (bgcolor.r * alpha);
					g = (bgcolor.g * alpha);
					b = (bgcolor.b * alpha);
					color = RGB15(r/256,g/256,b/256) | BIT(15);
				} else {
					u8 l;
					if (invert) l = a >> 3;
					else l = (255-a) >> 3;
					color = RGB15(l,l,l) | BIT(15);
				}
				screen[sy*display.height+sx] = color;
			}
		}
	}
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

void Text::PrintString(const char *s)
{
	//! Render a character string with current typesetter state.
	u8 i = 0;
	while (i < strlen(s)) {
		switch (s[i]) {
			case '\n':
			PrintNewLine();
			i++;
			break;
			default:
			u32 c;
			i += GetCharCode(&(s[i]), &c);
			PrintChar(c);
			break;
		}
	}
}

void Text::PrintStats() {
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
	SetPen(16, display.height-32);
	PrintString(msg);

	SetInvert(invert);
	SetPixelSize(ps);
	screen = s;
	SetPen(x,y);
}

void Text::ClearScreen(u16 *screen, u8 r, u8 g, u8 b)
{
	for (int i=0; i<display.height*display.height; i++)
		screen[i] = RGB15(r,g,b)|BIT(15);
}

void Text::PrintSplash(u16 *screen)
{
	u8 size = GetPixelSize();
	u16* s = GetScreen();
	
	SetScreen(screen);
	drawstack(screen);
	char msg[128];
	sprintf(msg,"%s",VERSION);
	PrintStatusMessage(msg);
	
	SetPixelSize(size);
	SetInvert(invert);
	SetScreen(s);
}

void Text::SetFont(u8 style, const char *filename)
{
	styles[style].face_index = 0;
	auto p = styles[style].file_path;
	strcpy(p, FONTDIR);
	strcat(p, "/");
	strcat(p, filename);
}

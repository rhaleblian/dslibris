#include <nds.h>
#include <fat.h>
#include "font.h"
#include "main.h"

#define MAXGLYPHS 128

extern u16 *fb,*screen0,*screen1;

FT_Library 		library;
FT_Face    		face;
FT_GlyphSlotRec glyphs[MAXGLYPHS];
FT_Vector		pen;
FT_Error   		error;

// accessors

int tsGetHeight(void) { return (face->size->metrics.height >> 6); }
void tsGetPen(int *x, int *y) { *x = pen.x; *y = pen.y; }
void tsSetPen(int x, int y) { pen.x = x; pen.y = y; }

// initialization

void tsInitPen(void) {
	pen.x = MARGINLEFT;
	pen.y = MARGINTOP + (face->size->metrics.height >> 6);
}

int tsInitDefault(void) {
  FT_Init_FreeType(&library);
  if(FT_New_Face(library, "/data/frutiger.ttf", 0, &face)) return -2;
	FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	FT_Set_Pixel_Sizes(face, 0, PIXELSIZE);
		
	// cache glyphs. glyphs[] will contain all the bitmaps.
	// using FirstChar() and NextChar() would be more robust.
	// TODO also cache kerning and transformations.
	int i;
	for(i=0;i<MAXGLYPHS;i++) {
		FT_Load_Char(face, i, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL);
		FT_GlyphSlot src = face->glyph;
		FT_GlyphSlot dst = &glyphs[i];
		int x = src->bitmap.rows;
		int y = src->bitmap.width;
		dst->bitmap.buffer = malloc(x*y);
		memcpy(dst->bitmap.buffer, src->bitmap.buffer, x*y);
		dst->bitmap.rows = src->bitmap.rows;
		dst->bitmap.width = src->bitmap.width;
		dst->bitmap_top = src->bitmap_top;
		dst->bitmap_left = src->bitmap_left;	
		dst->advance = src->advance;
	}
	
	tsInitPen();
	return(0);
}

// drawing

void tsChar(int code) {
	// draw an ASCII character,
	// with the current glyphs,
	// into the current buffer,
	// at the current pen position.
	FT_GlyphSlot glyph = &glyphs[code];
	FT_Bitmap bitmap = glyph->bitmap;
	int bx = glyph->bitmap_left;
	int by = glyph->bitmap_top;
	int gx, gy;
	for(gy=0; gy<bitmap.rows; gy++) {
		for(gx=0; gx<bitmap.width; gx++) {
			/* get antialiased value */
			int a = bitmap.buffer[gy*bitmap.width+gx];
			if(a) {
				u8 sx = (pen.x+gx+bx);
				u8 sy = (pen.y+gy-by);			
				int l = (255-a) >> 3;
				fb[sy*SCREEN_WIDTH+sx] = RGB15(l,l,l) | BIT(15);
			}
		}
	}
	pen.x += glyph->advance.x >> 6;
}

int tsStartNewLine(void) {
	int height = face->size->metrics.height >> 6;
	pen.x = MARGINLEFT;
	pen.y += height + LINESPACING;
	if(pen.y > (PAGE_HEIGHT - MARGINBOTTOM)) {
		if(fb == screen0) {
			fb = screen1;
			pen.y = MARGINTOP + height;
		} else return(1);
	}
	return(0);
}

void tsString(u8 *string) {
	// draw an ASCII string starting at the pen position.
	int c, i;
	for(i=0;i<strlen((char *)string);i++) {
		c = (int)string[i];
		if(c == '\n') tsStartNewLine();
		else tsChar(c);
	}
}

#include <nds.h>
#include "font.h"

FT_Library library;
FT_Error   error;
FT_Face    face;
FT_GlyphSlotRec glyphs[128];
extern FT_Vector pen;
extern u16 *fb;

void drawchar(int code, FT_Vector *pen) {
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
				u8 sx = (pen->x+gx+bx);
				u8 sy = (pen->y+gy-by);			
				int l = (255-a) >> 3;
				fb[sy*SCREEN_WIDTH+sx] = RGB15(l,l,l) | BIT(15);
			}
		}
	}
	pen->x += glyph->advance.x >> 6;
}

void drawstring(char *string, FT_Vector *pen) {
	// draw an ASCII string starting at the pen position.
	int c, i;
	for(i=0;i<strlen(string);i++) {
		c = (int)string[i];
		drawchar(c, pen);
	}
}

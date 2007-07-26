#include <nds.h>
#include "ui.h"
#include "font.h"
#include <stdio.h>

extern FT_Face    face;

void initbutton(button_t *b) {
	b->origin.x = 0;
	b->origin.y = 0;
	b->extent.x = 120;
	b->extent.y = 20;
	strcpy(b->text, "");
}

void labelbutton(button_t *b, char *text) {
	strncpy(b->text,text,63);
}

void movebutton(button_t *b, int x, int y) {
	b->origin.x = x;
	b->origin.y = y;
}

void drawbutton(button_t *b, u16 *fb, int highlight) {
	int x; int y;
	coord_t ll, ur;
	ll.x = b->origin.x;
	ll.y = b->origin.y;
	ur.x = b->origin.x + b->extent.x;
	ur.y = b->origin.y + b->extent.y;

	u16 color;
	if(highlight) {
		color = RGB15(0,15,0) | BIT(15);
		for(y=ll.y; y<ur.y; y++) {
			for(x=0; x<10; x++) {
				fb[y*SCREEN_WIDTH + x] = color;
				fb[y*SCREEN_WIDTH + x] = color;
			}
		}
	}
	FT_Vector textpos;
	textpos.x = ll.x+10;
	textpos.y = ll.y + (face->size->metrics.height >> 6);
	drawstring(b->text, &textpos);
}

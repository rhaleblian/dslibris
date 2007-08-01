#include <nds.h>
#include <stdio.h>
#include "ui.h"
#include "font.h"

void initbutton(button_t *b) {
	b->origin.x = 0;
	b->origin.y = 0;
	b->extent.x = 191;
	b->extent.y = 32;
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
	coord_t ul, lr;
	u16 color = RGB15(15,15,15) | BIT(15);

	ul.x = b->origin.x;
	ul.y = b->origin.y;
	lr.x = b->origin.x + b->extent.x;
	lr.y = b->origin.y + b->extent.y;

	for(x=ul.x;x<lr.x;x++) {
		fb[ul.y*SCREEN_WIDTH + x] = color;
		fb[lr.y*SCREEN_WIDTH + x] = color;
	}
	for(y=ul.y;y<lr.y;y++) {
		fb[y*SCREEN_WIDTH + ul.x] = color;
		fb[y*SCREEN_WIDTH + lr.x] = color;
	}
	if(highlight) {
		for(x=ul.x;x<lr.x;x++) {
			fb[(ul.y+1)*SCREEN_WIDTH + x] = color;
			fb[(lr.y-1)*SCREEN_WIDTH + x] = color;
		}
		for(y=ul.y;y<lr.y;y++) {
			fb[y*SCREEN_WIDTH + (ul.x+1)] = color;
			fb[y*SCREEN_WIDTH + (lr.x-1)] = color;
		}
	}
	tsSetPen(ul.x+10, ul.y + tsGetHeight());
	tsString(b->text);
}

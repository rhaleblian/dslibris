#include <nds.h>
#include <stdio.h>
#include "Button.h"

void Button::Init(Text *typesetter) {
	ts = typesetter;
	origin.x = 0;
	origin.y = 0;
	extent.x = 192;
	extent.y = 32;
	strcpy((char*)text, "");
}

void Button::Label(char *labeltext) {
	strncpy((char*)text,(char*)labeltext,63);
}

void Button::Move(u16 x, u16 y) {
	origin.x = x;
	origin.y = y;
}

void Button::Draw(u16 *fb, bool highlight) {
	u16 x;
	u16 y;
	coord_t ul, lr;
	ul.x = origin.x;
	ul.y = origin.y;
	lr.x = origin.x + extent.x;
	lr.y = origin.y + extent.y;
	if (highlight) {
		for (y=ul.y;y<lr.y;y++) {
			for (x=ul.x;x<lr.x;x++) {
				fb[y*SCREEN_WIDTH + x] = RGB15(31,31,31) | BIT(15);
			}
		}
	}

	u16 bordercolor = RGB15(15,15,15) | BIT(15);
	for (x=ul.x;x<lr.x;x++) {
		fb[ul.y*SCREEN_WIDTH + x] = bordercolor;
		fb[lr.y*SCREEN_WIDTH + x] = bordercolor;
	}
	for (y=ul.y;y<lr.y;y++) {
		fb[y*SCREEN_WIDTH + ul.x] = bordercolor;
		fb[y*SCREEN_WIDTH + lr.x] = bordercolor;
	}

	ts->SetInvert(!highlight);
	ts->GetPen(&x,&y);
	ts->SetPen(ul.x+10, ul.y + ts->GetHeight());
	ts->PrintString((const char*)text);
	ts->SetPen(x,y);
	ts->SetInvert(!highlight);
}

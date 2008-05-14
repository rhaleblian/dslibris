#include <nds.h>
#include <stdio.h>
#include "Button.h"

Button::Button() {
}

void Button::Init(Text *typesetter) {
	ts = typesetter;
	origin.x = 2;
	origin.y = 0;
	extent.x = 188;
	extent.y = 32;
	strcpy((char*)text, "");
}

void Button::Label(const char *labeltext) {
	strncpy((char*)text,(char*)labeltext,MAXPATHLEN);
}

void Button::Move(u16 x, u16 y) {
	origin.x = x;
	origin.y = y;
}

void Button::Resize(u16 x, u16 y) {
	extent.x = x;
	extent.y = y;
}

void Button::Draw(u16 *fb, bool highlight) {
	coord_t ul, lr;
	ul.x = origin.x;
	ul.y = origin.y;
	lr.x = origin.x + extent.x;
	lr.y = origin.y + extent.y;

	u16 x;
	u16 y;

	u16 bgcolor;
	if(highlight) bgcolor = RGB15(31,31,31) | BIT(15);
	else bgcolor = RGB15(0,0,0) | BIT(15);
	for (y=ul.y;y<lr.y;y++) {
		for (x=ul.x;x<lr.x;x++) {
			fb[y*SCREEN_WIDTH + x] = bgcolor;
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

	bool invert = ts->GetInvert();
	ts->SetScreen(fb);
	ts->SetInvert(!highlight);
	ts->GetPen(&x,&y);
	ts->SetPen(ul.x+10, ul.y + ts->GetHeight());
	ts->PrintString((const char*)text);
	ts->SetPen(x,y);
	ts->SetInvert(invert);
}
bool Button::EnclosesPoint(u16 x, u16 y)
{
	if (x > origin.x && 
		y > origin.y && 
		x < origin.x + extent.x && 
		y < origin.y + extent.y) return true;
	return false;
}

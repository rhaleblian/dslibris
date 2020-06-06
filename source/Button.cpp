/* 

dslibris - an ebook reader for the Nintendo DS.

 Copyright (C) 2007-2008 Ray Haleblian (ray23@sourceforge.net)

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

#include <nds.h>
#include <stdio.h>
#include "Button.h"

Button::Button() {
}

void Button::Init(Text *typesetter) {
	ts = typesetter;
	origin.x = 0;
	origin.y = 0;
	extent.x = 192;
	extent.y = 32;
	text = "";
	text2 = "";
}

void Button::Label(const char *s) {
	std::string str = s;	
	SetLabel(str);
}

void Button::SetLabel(std::string &s) {
	text = s;
}

void Button::SetLabel2(std::string s) {
	text2 = s;
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
	if(highlight) bgcolor = RGB15(31,31,15) | BIT(15);
	else bgcolor = RGB15(30,30,30) | BIT(15);
	for (y=ul.y+1;y<lr.y-1;y++) {
		for (x=ul.x+1;x<lr.x-1;x++) {
			fb[y*SCREEN_WIDTH + x] = bgcolor;
		}
	}

	// u16 bordercolor = RGB15(22,22,22) | BIT(15);
	// for (x=ul.x;x<lr.x;x++) {
	// 	fb[ul.y*SCREEN_WIDTH + x] = bordercolor;
	// 	fb[lr.y*SCREEN_WIDTH + x] = bordercolor;
	// }
	// for (y=ul.y;y<lr.y;y++) {
	// 	fb[y*SCREEN_WIDTH + ul.x] = bordercolor;
	// 	fb[y*SCREEN_WIDTH + lr.x-1] = bordercolor;
	// }

	bool invert = ts->GetInvert();
	ts->SetScreen(fb);
	ts->SetInvert(false);
	ts->GetPen(&x,&y);

	ts->SetPen(ul.x+6, ul.y + ts->GetHeight());
	if(highlight) ts->usebgcolor = true;

	// FIXME request a string fitting into the bounding box instead.
	ts->SetPixelSize(ts->GetPixelSize()+1);
	if(text.length() > 30)
		ts->PrintString((const char*)text.substr(0,30).append("...").c_str(),
			TEXT_STYLE_BROWSER);
	else
		ts->PrintString((const char*)text.c_str(), TEXT_STYLE_BROWSER);
	ts->SetPixelSize(ts->GetPixelSize()-1);

	if (text2.length()) {
		ts->SetPixelSize(ts->GetPixelSize()-1);
		ts->SetPen(ul.x+6, ts->GetPenY()+ts->GetHeight());
		ts->PrintString((const char *)text2.c_str(), TEXT_STYLE_BROWSER);
		ts->SetPixelSize(ts->GetPixelSize()+1);
	}

	ts->usebgcolor = false;
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

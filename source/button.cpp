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
#include "button.h"

Button::Button() {}

void Button::Init(Text *typesetter) {
	ts = typesetter;
	draw_border = true;
	origin.x = 0;
	origin.y = 0;
	extent.x = 192;
	extent.y = 34;
	style = BUTTON_STYLE_BOOK;
	text.pixelsize = 12;
	text.style = TEXT_STYLE_BROWSER;
	text1 = "";
	text2 = "";
}

void Button::Label(const char *s) {
	std::string str = s;
	SetLabel(str);
}

void Button::SetLabel1(std::string s) {
	text1 = s;
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

void Button::Draw(u16 *screen, bool highlight) {
	// push state
	int  save_pixelsize = ts->GetPixelSize();
	bool save_invert = ts->GetInvert();
	auto save_screen = ts->GetScreen();
	auto save_style = ts->GetStyle();
	auto save_usebgcolor = ts->usebgcolor;

	u16 x, y;
	coord_t ul, lr;
	ul.x = origin.x;
	ul.y = origin.y;
	lr.x = origin.x + extent.x;
	lr.y = origin.y + extent.y;
	int w = ts->display.height;  // no really
	// char msg[64]; sprintf(msg, "%d\n", w); app->PrintStatus(msg);
	u16 bgcolor = RGB15(30,30,30)|BIT(15);
	if(highlight) bgcolor = RGB15(31,31,15)|BIT(15);
	if(highlight) ts->usebgcolor = true;

	ts->SetScreen(screen);
	ts->SetInvert(false);
	ts->SetStyle(text.style);

	for (y=ul.y+1;y<lr.y-1;y++) {
		for (x=ul.x+1;x<lr.x-1;x++) {
			screen[y*w + x] = bgcolor;
		}
	}

	if (draw_border) {
		u16 bordercolor = RGB15(22,22,22)|BIT(15);
		for (int x=ul.x;x<lr.x;x++) {
			screen[ul.y*w + x] = bordercolor;
			screen[lr.y*w + x] = bordercolor;
		}
		for (int y=ul.y;y<lr.y;y++) {
			screen[y*w + ul.x] = bordercolor;
			screen[y*w + lr.x-1] = bordercolor;
		}
	}

	if (text1.length()) {
		const int s1 = style ? 1 : -1;
		ts->SetPixelSize(text.pixelsize+s1);
		ts->SetPen(ul.x+6, ul.y+ts->GetHeight());
		u8 len = ts->GetCharCountInsideWidth(text1.c_str(),
			text.style, lr.x-ul.x-4);
		ts->PrintString(text1.substr(0, len).c_str(),
			text.style);
	}

	if (text2.length()) {
		const int s2 = style ? -1 : 0;
		ts->SetPixelSize(text.pixelsize+s2);
		ts->SetPen(ul.x+6, ts->GetPenY()+ts->GetHeight());
		ts->PrintString(text2.c_str(), text.style);
	}

	// pop state
	ts->SetInvert(save_invert);
	ts->SetPixelSize(save_pixelsize);
	ts->SetScreen(save_screen);
	ts->SetStyle(save_style);
	ts->usebgcolor = save_usebgcolor;
}

bool Button::EnclosesPoint(u16 x, u16 y)
{
	if (x > origin.x && 
		y > origin.y && 
		x < origin.x + extent.x && 
		y < origin.y + extent.y) return true;
	return false;
}

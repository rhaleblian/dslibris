/*
 Copyright (C) 2007-2009 Ray Haleblian
 
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
 
 To contact the copyright holder: rayh23@sourceforge.net
 */

#include "Page.h"
#include <string.h>
#include <list>

Page::Page(Book *b)
{
	book = b;
	buf = NULL;
	length = 0;
	start = 0;
}

Page::~Page()
{
	if(buf) delete buf;
}

u8 Page::SetBuffer(u8 *src, u16 len)
{
	//! Write to offscreen buffer. NYI
	if(buf) delete buf;
	buf = new u8[len];
	strncpy((char*)buf, (char*)src, len);
	length = len;
	return 0;
}

void Page::Cache(FILE *fp)
{
	if(!buf) return;
	fwrite((const char*)buf, 1, strlen((char*)buf), fp);
}

#if 0
void Page::Draw()
{
	Text *ts = book->app->ts;
	if(ts) Draw(ts);
}
#endif

void Page::Draw(Text *ts)
{
	//! Write directly to video memory, for both screens.
	ts->SetScreen(ts->screenleft);
	ts->ClearScreen();
	ts->SetScreen(ts->screenright);
	ts->ClearScreen();
	ts->SetScreen(ts->screenleft);
	ts->InitPen();
	ts->linebegan = false;
	ts->italic = false;
	ts->bold = false;
	
	u16 i=0;
	while (i<length)
	{
		u32 c = buf[i];
		if (c == '\n')
		{
			// line break, page breaking if necessary
			i++;
			
			if (ts->GetPenY() + ts->GetHeight() + ts->linespacing 
				> ts->display.height - ts->margin.bottom)
			{
				if(ts->GetScreen() == ts->screenleft) {
					ts->SetScreen(ts->screenright);
					ts->InitPen();
					ts->linebegan = false;
				}
				else
					break;
			}
			else if (ts->linebegan) {
				ts->PrintNewLine();
			}
		} else if (c == TEXT_BOLD_ON) {
			i++;
			ts->bold = true;
		} else if (c == TEXT_BOLD_OFF) {
			i++;
			ts->bold = false;
		} else if (c == TEXT_ITALIC_ON) {
			i++;
			ts->italic = true;
		} else if (c == TEXT_ITALIC_OFF) {
			i++;
			ts->italic = false;
		} else {
			if (c > 127)
				i+=ts->GetCharCode((char*)&(buf[i]),&c);
			else
				i++;
			
			// TODO: there is such a thing as bold italic.
			if (ts->italic)
				ts->PrintChar(c, TEXT_STYLE_ITALIC);
			else if (ts->bold)
				ts->PrintChar(c, TEXT_STYLE_BOLD);
			else
				ts->PrintChar(c, TEXT_STYLE_NORMAL);
			
			ts->linebegan = true;
		}
	}
	DrawNumber(ts);
}

void Page::DrawNumber(Text *ts)
{
	//! Draw page number on current screen.
	char msg[128];
	
	// Find out if the page is bookmarked or not
	bool isBookmark = false;
	u16 pagecurrent = book->GetPosition();
	u16 pagecount = book->GetPageCount();
	std::list<u16>* bookmarks = book->GetBookmarks();
	for (std::list<u16>::iterator i = bookmarks->begin(); 
		 i != bookmarks->end(); i++) {
		if (*i == pagecurrent)
		{
			isBookmark = true;
			break;
		}
	}
	if (isBookmark) {
		if(pagecount == 1)
			sprintf((char*)msg,"[ %d* ]",pagecurrent+1);
		else if(pagecurrent == 0) 
			sprintf((char*)msg,"[ %d* >",pagecurrent+1);
		else if(pagecurrent == pagecount-1)
			sprintf((char*)msg,"< %d* ]",pagecurrent+1);
		else
			sprintf((char*)msg,"< %d* >",pagecurrent+1);
	} else {
		if(pagecount == 1)
			sprintf((char*)msg,"[ %d ]",pagecurrent+1);
		else if(pagecurrent == 0) 
			sprintf((char*)msg,"[ %d >",pagecurrent+1);
		else if(pagecurrent == pagecount-1)
			sprintf((char*)msg,"< %d ]",pagecurrent+1);
		else
			sprintf((char*)msg,"< %d >",pagecurrent+1);
	}

	// Position page number in horizontal proportion
	// to our current progress in the book.
	int stringwidth = ts->GetStringAdvance(msg);
	int region = ts->display.width - ts->margin.left - ts->margin.right - stringwidth;
	int location;
	if(pagecount == 1)
		location = (ts->display.width/2) - (stringwidth/2);
	else
		location = ts->margin.left
			+ (int)((float)region * (float)pagecurrent / (float)(pagecount-1));

	ts->SetScreen(ts->screenright);
	ts->SetPen((u8)location,250);
	ts->PrintString(msg);
}

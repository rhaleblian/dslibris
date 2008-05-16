#include <errno.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>

#include <expat.h>

#include <fat.h>
#include <nds/registers_alt.h>
#include <nds/reload.h>

#include "ndsx_brightness.h"
#include "types.h"
#include "main.h"
#include "parse.h"
#include "App.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"

#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)

void App::HandleEventInBook()
{
	//Keishava - assigned the right and left direction keys
	//to scroll fast if the key is kept depressed, instead of
	//stopping at the previous / next page if KEY_UP/KEY_DOWN
	//is pressed.
	//Aaron - Removed excessive preference saving
	
	if (keysUp() & (KEY_RIGHT | KEY_LEFT))
	{
		// Only save preferences once the fastscrolling is done
		prefs->Write();
	}
	
	if (keysHeld() & KEY_RIGHT)
	{
		if (pagecurrent < pagecount)
		{
			pagecurrent++;
			page_draw(&pages[pagecurrent]);
			books[bookcurrent].SetPosition(pagecurrent);
		}
	}
	else if (keysHeld() & KEY_LEFT)
	{
		if (pagecurrent > 0)
		{
			pagecurrent--;
			page_draw(&pages[pagecurrent]);
			books[bookcurrent].SetPosition(pagecurrent);
		}
	}
	else if (keysDown() & (KEY_A | KEY_DOWN))
	{
		if (pagecurrent < pagecount)
		{
			pagecurrent++;
			page_draw(&pages[pagecurrent]);
			books[bookcurrent].SetPosition(pagecurrent);
			prefs->Write();
		}
	}

	else if (keysDown() & (KEY_B | KEY_UP))
	{
		if (pagecurrent > 0)
		{
			pagecurrent--;
			page_draw(&pages[pagecurrent]);
			books[bookcurrent].SetPosition(pagecurrent);
			prefs->Write();
		}
	}

	else if (keysDown() & KEY_X)
	{
		ts->SetInvert(!ts->GetInvert());
		page_draw(&pages[pagecurrent]);
		prefs->Write();
	}

	else if (keysDown() & KEY_Y)
	{
		CycleBrightness();
	}

	else if (keysDown() & KEY_START)
	{
		reopen = 0;
		prefs->Write();
		mode = APP_MODE_BROWSER;
		if(orientation) ts->PrintSplash(screen1);
		else ts->PrintSplash(screen0);
		browser_draw();
	}

	else if (keysDown() & KEY_TOUCH)
	{
		touchPosition touch = touchReadXY();
		if (touch.py < 96)
		{
			if (pagecurrent > 0) pagecurrent--;
		}
		else
		{
			if (pagecurrent < pagecount) pagecurrent++;
		}
		page_draw(&pages[pagecurrent]);
	}

	else if (keysDown() & KEY_SELECT)
	{
		// Toggle Bookmark
		Book* book = books + bookcurrent;
		std::list<u16>* bookmarks = book->GetBookmarks();
		
		bool found = false;
		for (std::list<u16>::iterator i = bookmarks->begin(); i != bookmarks->end(); i++) {
			if (*i == pagecurrent)
			{
				bookmarks->erase(i);
				found = true;
				break;
			}
        }
		
		if (!found)
		{
			bookmarks->push_back(pagecurrent);
			bookmarks->sort();
		}
		
		page_draw(&pages[pagecurrent]);
	}
	
	else if (keysDown() & (KEY_R | KEY_L))
	{
		// Bookmark Navigation
		Book* book = books + bookcurrent;
		std::list<u16>* bookmarks = book->GetBookmarks();
		
		if (!bookmarks->empty())
		{
			//Find the bookmark just after the current page
			if (keysDown() & KEY_R)
			{
				std::list<u16>::iterator i;
				for (i = bookmarks->begin(); i != bookmarks->end(); i++) {
					if (*i > pagecurrent)
						break;
				}
				
				if (i == bookmarks->end())
					i = bookmarks->begin();
				
				pagecurrent = *i;
			}
			else // KEY_L by process of elimination
			{
				std::list<u16>::reverse_iterator i;
				for (i = bookmarks->rbegin(); i != bookmarks->rend(); i++) {
					if (*i < pagecurrent)
						break;
				}
				
				if (i == bookmarks->rend())
					i = bookmarks->rbegin();
				
				pagecurrent = *i;
			}
			
			page_draw(&pages[pagecurrent]);
			book->SetPosition(pagecurrent);
			prefs->Write();
		}
	}
	
	// clock - only display when reading a book
	time_t tt = time(NULL);
	struct tm *tms = gmtime((const time_t *)&tt);
	char tmsg[6];
	sprintf(tmsg, "%02d:%02d", tms->tm_hour, tms->tm_min);
	u8 offset = marginleft;
	ts->SetScreen(screen0);
	ts->ClearRect(offset, 240, offset+30, 255);
	ts->SetPen(offset,250);
	ts->PrintString(tmsg);
}

u8 App::OpenBook(void)
{
	ts->SetScreen(screen1);
	ts->ClearScreen(screen1,0,0,0);
	ts->SetPen(marginleft,PAGE_HEIGHT/2);
	bool invert = ts->GetInvert();
	ts->SetInvert(true);
	ts->PrintString("[opening...]");
	ts->SetInvert(invert);
	swiWaitForVBlank();
	pagecount = 0;
	pagecurrent = 0;
	bookBold = false;
	bookItalic = false;
	page_init(&pages[pagecurrent]);
	ts->ClearCache();
	if (!books[bookselected].Parse(filebuf))
	{
		bookcurrent = bookselected;
		pagecurrent = books[bookselected].GetPosition();
		page_draw(&(pages[pagecurrent]));
		prefs->Write();
		return 0;
	}
	else
		return 255;
}

void App::page_init(page_t *page)
{
	page->length = 0;
	page->buf = NULL;
}

u8 App::page_getjustifyspacing(page_t *page, u16 i)
{
	/** full justification. get line advance, count spaces,
	    and insert more space in spaces to reach margin.
	    returns amount of space to add per-character. **/

	u8 spaces = 0;
	u8 advance = 0;
	u8 j,k;

	/* walk through leading spaces */
	for (j=i;j<page->length && page->buf[j]==' ';j++);

	/* find the end of line */
	for (j=i;j<page->length && page->buf[j]!='\n';j++)
	{
		u16 c = page->buf[j];
		advance += ts->GetAdvance(c);

		if (page->buf[j] == ' ') spaces++;
	}

	/* walk back through trailing spaces */
	for (k=j;k>0 && page->buf[k]==' ';k--) spaces--;

	if (spaces)
		return((u8)((float)((PAGE_WIDTH-marginright-marginleft) - advance)
		            / (float)spaces));
	else return(0);
}


void App::parse_printerror(XML_Parser p)
{
	u16 *screen = ts->GetScreen();
	u16 x,y;
	ts->GetPen(x,y);

	char msg[256];
	sprintf(msg,"line %d, col %d: %s\n",
		(int)XML_GetCurrentLineNumber(p),
		(int)XML_GetCurrentColumnNumber(p),
		XML_ErrorString(XML_GetErrorCode(p)));
	Log(msg);

	ts->SetScreen(screen0);
	ts->InitPen();
	ts->ClearScreen();
	ts->PrintString(msg);

	ts->SetScreen(screen);
	ts->SetPen(x,y);
}

void App::parse_init(parsedata_t *data)
{
	data->stacksize = 0;
	data->book = NULL;
	data->page = NULL;
	data->pen.x = marginleft;
	data->pen.y = margintop;
}

void App::parse_push(parsedata_t *data, context_t context)
{
	data->stack[data->stacksize++] = context;
}

context_t App::parse_pop(parsedata_t *data)
{
	if (data->stacksize) data->stacksize--;
	return data->stack[data->stacksize];
}

bool App::parse_in(parsedata_t *data, context_t context)
{
	u8 i;
	for (i=0;i<data->stacksize;i++)
	{
		if (data->stack[i] == context) return true;
	}
	return false;
}

bool App::parse_pagefeed(parsedata_t *data, page_t *page)
{
	// called when we are at the end of one of the facing pages.

	bool pagedone = false;
	
	if (fb == screen1)
	{
		// we left the right page, save chars into this page.

		if (!page->buf)
		{
			page->buf = new u8[page->length];
			if (!page->buf)
			{
				ts->PrintString("[out of memory]\n");
				exit(-7);
			}
		}
		memcpy(page->buf,pagebuf,page->length * sizeof(u8));
		fb = screen0;
		pagedone = true;
	}
	else
	{
		fb = screen1;
		pagedone = false;
	}
	data->pen.x = marginleft;
	data->pen.y = margintop + ts->GetHeight();
	return pagedone;
}

void App::page_draw(page_t *page)
{
	ts->SetScreen(screen1);
	ts->ClearScreen();
	ts->SetScreen(screen0);
	ts->ClearScreen();
	ts->InitPen();
	bool linebegan = false;

	u16 i=0;
	while (i<page->length)
	{
		u32 c = page->buf[i];
		if (c == '\n')
		{
			// line break, page breaking if necessary
			i++;

			if (ts->GetPenY() + ts->GetHeight() + linespacing 
				> PAGE_HEIGHT - marginbottom)
			{
				if(ts->GetScreen() == screen0) {
					ts->SetScreen(screen1);
					ts->InitPen();
					linebegan = false;
				}
				else break;
			}
			else if (linebegan) ts->PrintNewLine();
		} else if (c == TEXT_BOLD) {
			i++;
			
			bookBold = !bookBold;
		} else if (c == TEXT_ITALIC) {
			i++;
			
			bookItalic = !bookItalic;
		}
		else
		{
			if (c > 127)
				i+=ts->GetCharCode((char*)&(page->buf[i]),&c);
			else
				i++;
			
			if (bookItalic)
				ts->PrintChar(c, ts->italicFace);
			else if (bookBold)
				ts->PrintChar(c, ts->boldFace);
			else
				ts->PrintChar(c);
			
			linebegan = true;
		}
	}

	// page number
	
	char msg[9];
	strcpy(msg,"");
	
	// Find out if the page is bookmarked or not
	bool isBookmark = false;
	Book* book = books + bookselected;
	std::list<u16>* bookmarks = book->GetBookmarks();
	for (std::list<u16>::iterator i = bookmarks->begin(); i != bookmarks->end(); i++) {
		if (*i == pagecurrent)
		{
			isBookmark = true;
			break;
		}
	}
	// TODO: Decide where to display the asterisk and use a more elegant solution
	if (isBookmark) {
		if(pagecurrent == 0) 
			sprintf((char*)msg,"[ *%d >",pagecurrent+1);
		else if(pagecurrent == pagecount)
			sprintf((char*)msg,"< *%d ]",pagecurrent+1);
		else
			sprintf((char*)msg,"< *%d >",pagecurrent+1);
	} else {
		if(pagecurrent == 0) 
			sprintf((char*)msg,"[ %d >",pagecurrent+1);
		else if(pagecurrent == pagecount)
			sprintf((char*)msg,"< %d ]",pagecurrent+1);
		else
			sprintf((char*)msg,"< %d >",pagecurrent+1);
	}
	ts->SetScreen(screen1);
	u8 offset = (u8)((PAGE_WIDTH-marginleft-marginright-(ts->GetAdvance(40)*7))
		* (pagecurrent / (float)pagecount));
	ts->SetPen(marginleft+offset,250);
	ts->PrintString(msg);
}

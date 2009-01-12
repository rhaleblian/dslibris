#include <errno.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>

#include <expat.h>

#include <fat.h>

#include "main.h"
#include "parse.h"
#include "App.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"

#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)

//! Book-related methods for App class.

void App::HandleEventInBook()
{
	uint32 keys = keysDownRepeat();

	if (keys & (KEY_A|key.up|key.down))
	{
		if (pagecurrent < pagecount)
		{
			pagecurrent++;
			page_draw(&pages[pagecurrent]);
			books[bookcurrent]->SetPosition(pagecurrent);
		}
	}

	else if (keys & (KEY_B|key.r|key.up))
	{
		if(pagecurrent > 0)
		{
			pagecurrent--;
			page_draw(&pages[pagecurrent]);
			books[bookcurrent]->SetPosition(pagecurrent);
		}
	}

	keys = keysDown();

	if (keys & KEY_X)
	{
	}

	else if (keys & KEY_Y)
	{
		CycleBrightness();
	}

	else if (keys & KEY_START)
	{
		// return to browser.
		mode = APP_MODE_BROWSER;
		if(orientation) lcdSwap();
		ts->PrintSplash(screenleft);
		reopen = false; // Resume in browser, not in book.
		prefs->Write();
		browser_draw();
	}

	else if (keys & KEY_TOUCH)
	{
		// Turn page on touch.
		touchPosition touch;
 		touchRead(&touch);
		if ((orientation && touch.py > 95) || (touch.py < 96))
		{
			if (pagecurrent > 0) pagecurrent--;
		}
		else
		{
			if (pagecurrent < pagecount) pagecurrent++;
		}
		page_draw(&pages[pagecurrent]);
	}

	else if (keys & KEY_SELECT)
	{
		// Toggle Bookmark
		Book* book = books[bookcurrent];
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
	else if (keys & (key.right | key.right))
	{
		// Bookmark Navigation
		Book* book = books[bookcurrent];
		std::list<u16>* bookmarks = book->GetBookmarks();
	
		if (!bookmarks->empty())
		{
			//Find the bookmark just after the current page
			if (keys & key.left)
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
			else // KEY_OTHER by process of elimination
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
		}
	}

	if(keysUp()) prefs->Write();
}

u8 App::OpenBook(void)
{
	//! Attempt to open book indexed by bookselected.
	
	PrintStatus("[opening book...]");
	swiWaitForVBlank();

	pagecount = 0;
	pagecurrent = 0;
	bookBold = false;
	bookItalic = false;
	page_init(&pages[pagecurrent]);
	ts->ClearCache();
	ts->SetScreen(screenleft);
	const char *filename = books[bookselected]->GetFileName();

	const char *c; 	// will point to the file's extension.
	for (c=filename;c!=filename+strlen(filename) && *c!='.';c++);
	if (books[bookselected]->Parse(filebuf))
	{
		PrintStatus("[could not open book]");
		return 255;
	}
	PrintStatus("[book opened]");
	bookcurrent = bookselected;
	pagecurrent = books[bookselected]->GetPosition();
	page_draw(&(pages[pagecurrent]));
	prefs->Write();
	return 0;
}

void App::page_init(page_t *page)
{
	//! Intialize a page struct. Call before parsing into a new page.
	page->length = 0;
	page->buf = NULL;
}

u8 App::page_getjustifyspacing(page_t *page, u16 i)
{
	/*! Return interword space required for full justification.
		Get line advance, count spaces,
	    and insert more space in spaces to reach margin.
		BROKEN!
	    \return Amount of space between each word for the line
		starting at i. */

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
	sprintf(msg,"%d:%d: %s\n",
		(int)XML_GetCurrentLineNumber(p),
		(int)XML_GetCurrentColumnNumber(p),
		XML_ErrorString(XML_GetErrorCode(p)));
	Log(msg);
	PrintStatus(msg);
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
	u16 *screen = ts->GetScreen();

	if (screen == screenright)
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
		ts->SetScreen(screenleft);
		pagedone = true;
	}
	else
	{
		ts->SetScreen(screenright);
		pagedone = false;
	}
	data->pen.x = marginleft;
	data->pen.y = margintop + ts->GetHeight();
	return pagedone;
}

void App::page_draw(page_t *page)
{
	ts->SetScreen(screenright);
	ts->ClearScreen();
	ts->SetScreen(screenleft);
	ts->ClearScreen();
	ts->InitPen();
	bool linebegan = false;
	
	bookItalic = false;
	bookBold = false;

	u16 i=0;
	while (i<page->length)
	{
		u32 c = page->buf[i];
		if (c == '\n')
		{
			// line break, page breaking if necessary
			i++;

			if (ts->GetPenY() + ts->GetHeight() + linespacing > PAGE_HEIGHT - marginbottom)
			{
				if(ts->GetScreen() == screenleft) {
					ts->SetScreen(screenright);
					ts->InitPen();
					linebegan = false;
				}
				else
					break;
			}
			else if (linebegan) {
				ts->PrintNewLine();
			}
		} else if (c == TEXT_BOLD_ON) {
			i++;
			bookBold = true;
		} else if (c == TEXT_BOLD_OFF) {
			i++;
			bookBold = false;
		} else if (c == TEXT_ITALIC_ON) {
			i++;
			bookItalic = true;
		} else if (c == TEXT_ITALIC_OFF) {
			i++;
			bookItalic = false;
		} else {
			if (c > 127)
				i+=ts->GetCharCode((char*)&(page->buf[i]),&c);
			else
				i++;
			
			if (bookItalic)
				ts->PrintChar(c, TEXT_STYLE_ITALIC);
			else if (bookBold)
				ts->PrintChar(c, TEXT_STYLE_BOLD);
			else
				ts->PrintChar(c, TEXT_STYLE_NORMAL);
			
			linebegan = true;
		}
	}

	// page number
	
	// Find out if the page is bookmarked or not
	bool isBookmark = false;
	Book* book = books[bookselected];
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
			sprintf((char*)msg,"[ %d* >",pagecurrent+1);
		else if(pagecurrent == pagecount)
			sprintf((char*)msg,"< %d* ]",pagecurrent+1);
		else
			sprintf((char*)msg,"< %d* >",pagecurrent+1);
	} else {
		if(pagecurrent == 0) 
			sprintf((char*)msg,"[ %d >",pagecurrent+1);
		else if(pagecurrent == pagecount)
			sprintf((char*)msg,"< %d ]",pagecurrent+1);
		else
			sprintf((char*)msg,"< %d >",pagecurrent+1);
	}
	ts->SetScreen(screenright);
	u8 offset = (u8)((PAGE_WIDTH-marginleft-marginright-(ts->GetAdvance(40)*7))
		* (pagecurrent / (float)pagecount));
	ts->SetPen(marginleft+offset,250);
	ts->PrintString(msg);
}


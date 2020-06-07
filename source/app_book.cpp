#include <errno.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>

#include <expat.h>

#include <fat.h>
#include <nds/bios.h>

#include "types.h"
#include "main.h"
#include "parse.h"
#include "app.h"
#include "book.h"
#include "button.h"
#include "text.h"

//! Book-related methods for App class.

void App::HandleEventInBook()
{
	u16 pagecurrent = bookcurrent->GetPosition();
	u16 pagecount = bookcurrent->GetPageCount();

	if (key.downrepeat & (KEY_A|key.r|key.down))
	{
		// page forward.
		if (pagecurrent < pagecount-1)
		{
			pagecurrent++;
			bookcurrent->SetPosition(pagecurrent);
			bookcurrent->GetPage()->Draw(ts);
		}
	}

	else if (key.downrepeat & (KEY_B|key.l|key.up))
	{
		// page back.
		if(pagecurrent > 0)
		{
			pagecurrent--;
			bookcurrent->SetPosition(pagecurrent);
			bookcurrent->GetPage()->Draw(ts);
		}
	}

	uint32 keys = keysDown();

	if(keys){

		if (keys & KEY_X)
		{
			// toggle inverted text.
			ts->SetInvert(!ts->GetInvert()); 	 
			bookcurrent->GetPage()->Draw(ts);
		}

		else if (keys & KEY_Y)
		{
			// go to next level in brightness. 
			CycleBrightness();
			prefs->Write();
		}

		else if (keys & KEY_START)
		{
			// return to browser.
			bookcurrent->Close();
			bookcurrent = NULL;
			if(mode == APP_MODE_BOOK)
			{
				if(orientation) lcdSwap();
				mode = APP_MODE_BROWSER;
			}
			ts->PrintSplash(ts->screenleft);
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
				if (pagecurrent > 0)
				{
					pagecurrent--;
					bookcurrent->SetPosition(pagecurrent);
					bookcurrent->GetPage()->Draw(ts);
				}
			}
			else
			{
				if (pagecurrent < pagecount-1)
				{
					pagecurrent++;
					bookcurrent->SetPosition(pagecurrent);
					bookcurrent->GetPage()->Draw(ts);
				}
			}
		}

		else if (keys & KEY_SELECT)
		{
			// Toggle bookmark.
			std::list<u16>* bookmarks = bookcurrent->GetBookmarks();
	
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
	
			bookcurrent->GetPage()->Draw(ts);
		}
		else if (keys & (key.right | key.left))
		{
			// Navigate bookmarks.
			std::list<u16>* bookmarks = bookcurrent->GetBookmarks();
	
			if (!bookmarks->empty())
			{
				//Find the bookmark just after the current page
				if (keys & key.left)
				{
					std::list<u16>::iterator i;
					for (i = bookmarks->begin(); i != bookmarks->end(); i++) {
						if (*i > bookcurrent->GetPosition())
							break;
					}
			
					if (i == bookmarks->end())
						i = bookmarks->begin();
			
					bookcurrent->SetPosition(*i);
				}
				else // KEY_OTHER by process of elimination
				{
					std::list<u16>::reverse_iterator i;
					for (i = bookmarks->rbegin(); i != bookmarks->rend(); i++) {
						if (*i < bookcurrent->GetPosition())
							break;
					}
			
					if (i == bookmarks->rend())
						i = bookmarks->rbegin();
			
					bookcurrent->SetPosition(*i);
				}
		
				bookcurrent->GetPage()->Draw(ts);
			}
		}

	}

	if(keysUp()) prefs->Write();
}

int App::GetBookIndex(Book *b)
{
	if (!b) return -1;
	vector<Book*>::iterator it;
	int i=0;
	for(it=books.begin(); it<books.end();it++,i++)
	{
		if(*it == b) return i;
	}
	return -1;
}

u8 App::OpenBook(void)
{
	//! Attempt to open book indicated by bookselected.

	if(!bookselected) return 254;	
	PrintStatus("opening book...");
	swiWaitForVBlank();

	const char *filename = bookselected->GetFileName();
	const char *c; 	// will point to the file's extension.
	for (c=filename;c!=filename+strlen(filename) && *c!='.';c++);
	
	if(bookcurrent) bookcurrent->Close();
	if (int err = bookselected->Open())
	{
		char msg[64];
		sprintf(msg, "could not open book (%d)",err);
		PrintStatus(msg);
		return 255;
	}
	PrintStatus("book opened");
	bookcurrent = bookselected;
	if(mode == APP_MODE_BROWSER) {
		if(orientation) lcdSwap();
		mode = APP_MODE_BOOK;
	}
	if(bookcurrent->GetPosition() >= bookcurrent->GetPageCount())
		bookcurrent->SetPosition(0);
	bookcurrent->GetPage()->Draw(ts);
	prefs->Write();
	ts->PrintStats();
	return 0;
}

void App::parse_error(XML_Parser p)
{
	char msg[128];
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
	data->pos = 0;
	data->book = NULL;
	data->prefs = NULL;
	data->screen = 0;
	data->pen.x = ts->margin.left;
	data->pen.y = ts->margin.top;
	data->linebegan = false;
	data->bold = false;
	data->italic = false;
	strcpy((char*)data->buf,"");
	data->cachefile = NULL;
	data->buflen = 0;
	data->status = 0;
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

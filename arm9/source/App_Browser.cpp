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

void App::HandleEventInBrowser()
{
	if (keysDown() & KEY_A)
	{
		AttemptBookOpen();
	}
	
	else if (keysDown() & KEY_Y)
	{
		CycleBrightness();
	}
	
	else if (keysDown() & KEY_SELECT)
	{
		mode = APP_MODE_PREFS;
		prefsSelected = 0;
		PrefsDraw();
	}
	
	else if (keysDown() & (KEY_LEFT | KEY_L))
	{
		if (bookselected < bookcount-1)
		{
			if (bookselected == browserstart+6) {
				browser_nextpage();
				browser_draw();
			} else {
				bookselected++;
				browser_redraw();
			}
		}
	}

	else if (keysDown() & (KEY_RIGHT | KEY_R))
	{
		if (bookselected > 0)
		{
			if(bookselected == browserstart) {
				browser_prevpage();
				browser_draw();
			} else {
				bookselected--;
				browser_redraw();
			}				
		}
	}

	else if (bookcurrent >= 0 && (keysDown() & (KEY_START | KEY_B)))
	{
		mode = APP_MODE_BOOK;
		page_draw(&(pages[pagecurrent]));
		reopen = 1;
		prefs->Write();
	}

	else if (keysDown() & KEY_TOUCH)
	{
		touchPosition touch = touchReadXY();
		touchPosition coord;
		u8 regionprev[2], regionnext[2];
		regionprev[0] = 0;		
		regionprev[1] = 16;
		regionnext[0] = 240;
		regionnext[1] = 255;

		if(!orientation)
		{
			coord.px = 256 - touch.px;
			coord.py = 192 - touch.py;
		} else {
			coord.px = touch.px;
			coord.py = touch.py;
		}

		if(coord.px > regionnext[0]
			&& coord.px < regionnext[1])
		{
			browser_nextpage();
			browser_draw();
		}
		else if(coord.px > regionprev[0]
			&& coord.px < regionprev[1])
		{
			if (browserstart > 6) {
				browser_prevpage();
				browser_draw();
			} else {
				mode = APP_MODE_PREFS;
				prefsSelected = 0;
				PrefsDraw();
			}
		} else {
			for(u8 i=browserstart;
				(i<bookcount) && (i<browserstart+7);
				i++) {
				if (buttons[i].EnclosesPoint(coord.py, coord.px))
				{
					bookselected = i;
					browser_draw();
					swiWaitForVBlank();
					AttemptBookOpen();
					break;
				}
			}
		}
	}	
}

void App::browser_init(void)
{
	u8 i;
	for (i=0;i<bookcount;i++)
	{
		buttons[i].Init(ts);
		buttons[i].Move(2,((i%7+1)*32)-16);
		if (strlen(books[i].GetTitle()))
			buttons[i].Label(books[i].GetTitle());
		else
			buttons[i].Label(books[i].GetFileName());
	}
	buttonprev.Init(ts);
	buttonprev.Move(2,0);
	buttonprev.Resize(188,16);
	buttonprev.Label("^");
	buttonnext.Init(ts);
	buttonnext.Move(2,240);
	buttonnext.Resize(188,16);
	buttonnext.Label("v");
	browserstart = (bookselected / 7) * 7;
}

void App::browser_nextpage()
{
	if(browserstart+7 < bookcount)
	{ 
		browserstart += 7;
		bookselected = browserstart;
	}
}

void App::browser_prevpage()
{
	if(browserstart-7 >= 0)
	{
		browserstart -= 7;
		bookselected = browserstart+6;
	}
}

void App::browser_draw(void)
{
	// save state.
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();

#ifndef GRIT
	u16* screen;
	if(orientation) screen = screen0;
	else screen = screen1;
 
	ts->SetScreen(screen);
	ts->ClearScreen(screen,0,0,0);
	ts->SetPixelSize(12);
	for (int i=browserstart;(i<bookcount) && (i<browserstart+7);i++)
	{
		buttons[i].Draw(screen,i==bookselected);
	}

	if(browserstart > 6)
		buttonprev.Label("^");
	else
		buttonprev.Label("Configure dslibris [Select]");
	
	buttonprev.Draw(screen,false);
	if(bookcount > browserstart+7)
		buttonnext.Draw(screen,false);
#endif

	// restore state.
	ts->SetInvert(invert);
	ts->SetPixelSize(size);
}

void App::browser_redraw()
{
	// only call this when incrementing or decrementing the
	// selected book; otherwise use browser_draw().

	// save state.
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();

#ifndef GRIT

	u16 *screen;
	if(orientation) screen = screen0;
	else screen = screen1;

	ts->SetScreen(screen);
	ts->SetPixelSize(12);
	buttons[bookselected].Draw(screen,true);
	if(bookselected > browserstart)
		buttons[bookselected-1].Draw(screen,false);
	if(bookselected < bookcount-1 && bookselected - browserstart < 6)
		buttons[bookselected+1].Draw(screen,false);

#endif

	// restore state.
	ts->SetInvert(invert);
	ts->SetPixelSize(size);
}

void App::AttemptBookOpen()
{
	// Just switch if the book is already open/parsed
	if(bookselected == bookcurrent) {
		mode = APP_MODE_BOOK;
		page_draw(&(pages[pagecurrent]));
		reopen = 1;
		prefs->Write();
	// Parse the selected book.
	} else if (!OpenBook()) {
		mode = APP_MODE_BOOK;
		reopen = 1;
		prefs->Write();
	// Fail...
	} else
		browser_draw();
}

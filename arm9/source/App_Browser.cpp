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
		// next book.
		if (bookselected < bookcount-1)
		{
			bookselected++;
			if (bookselected >= browserstart+APP_BROWSER_BUTTON_COUNT) {
				browser_nextpage();
				browser_draw();
			} else {
				browser_redraw();
			}
		}
	}

	else if (keysDown() & (KEY_RIGHT | KEY_R))
	{
		// previous book.
		if (bookselected > 0)
		{
			bookselected--;
			if(bookselected < browserstart) {
				browser_prevpage();
				browser_draw();
			} else {
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

		if(!orientation)
		{
			coord.px = 256 - touch.px;
			coord.py = touch.py;
		} else {
			coord.px = touch.px;
			coord.py = 192 - touch.py;
		}

		if(buttonnext.EnclosesPoint(coord.py, coord.px))
		{
			browser_nextpage();
			browser_draw();
		}
		else if(buttonprev.EnclosesPoint(coord.py, coord.px))
		{
			browser_prevpage();
			browser_draw();
		}
		else if(buttonprefs.EnclosesPoint(coord.py, coord.px))
		{
			if(mode != APP_MODE_PREFS) {
				mode = APP_MODE_PREFS;
				prefsSelected = 0;
				PrefsDraw();
			} else {
				mode = APP_MODE_BROWSER;
				browser_draw();
			}
		} else {
			for(u8 i=browserstart;
				(i<bookcount) &&
				(i<browserstart+APP_BROWSER_BUTTON_COUNT);
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
		buttons[i].Move(2,((i%APP_BROWSER_BUTTON_COUNT+1)*32)-16);
		if (strlen(books[i].GetTitle()))
			buttons[i].Label(books[i].GetTitle());
		else
			buttons[i].Label(books[i].GetFileName());
	}
	buttonprev.Init(ts);
	buttonprev.Move(2,240);
	buttonprev.Resize(60,16);
	buttonprev.Label("prev");
	buttonnext.Init(ts);
	buttonnext.Move(130,240);
	buttonnext.Resize(60,16);
	buttonnext.Label("next");
	buttonprefs.Init(ts);
	buttonprefs.Move(66,240);
	buttonprefs.Resize(60,16);
	buttonprefs.Label("prefs");

	browserstart = (bookselected / APP_BROWSER_BUTTON_COUNT)
		* APP_BROWSER_BUTTON_COUNT;
}

void App::browser_nextpage()
{
	if(browserstart+APP_BROWSER_BUTTON_COUNT < bookcount)
	{ 
		browserstart += APP_BROWSER_BUTTON_COUNT;
		bookselected = browserstart;
	}
}

void App::browser_prevpage()
{
	if(browserstart-APP_BROWSER_BUTTON_COUNT >= 0)
	{	
		browserstart -= APP_BROWSER_BUTTON_COUNT;
		bookselected = browserstart+APP_BROWSER_BUTTON_COUNT-1;
	}
}

void App::browser_draw(void)
{
	// save state.
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();

	u16* screen;
	if(orientation) screen = screen0;
	else screen = screen1;
 
	ts->SetScreen(screen);
	ts->SetInvert(false);
	ts->ClearScreen(screen,31,31,31);
	ts->SetPixelSize(PIXELSIZE);
	for (int i=browserstart;
		(i<bookcount) && (i<browserstart+APP_BROWSER_BUTTON_COUNT);
		i++)
	{
		buttons[i].Draw(screen,i==bookselected);
	}
	
	if(browserstart >= APP_BROWSER_BUTTON_COUNT)
		buttonprev.Draw(screen,false);
	if(bookcount >= browserstart+APP_BROWSER_BUTTON_COUNT)
		buttonnext.Draw(screen,false);

	buttonprefs.Draw(screen,false);

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

	u16 *screen;
	if(orientation) screen = screen0;
	else screen = screen1;

	ts->SetScreen(screen);
	ts->SetInvert(false);
	ts->SetPixelSize(PIXELSIZE);
	buttons[bookselected].Draw(screen,true);
	if(bookselected > browserstart)
		buttons[bookselected-1].Draw(screen,false);
	if(bookselected < bookcount-1 &&
		(bookselected - browserstart) < APP_BROWSER_BUTTON_COUNT-1)
		buttons[bookselected+1].Draw(screen,false);

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

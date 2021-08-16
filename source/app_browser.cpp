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

//#include "ndsx_brightness.h"
#include "types.h"
#include "main.h"
#include "parse.h"
#include "app.h"
#include "book.h"
#include "button.h"
#include "text.h"

#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)

void App::HandleEventInBrowser()
{
	uint32 keys = keysDown();
	
	if (keys & (KEY_A | key.down))
	{
		AttemptBookOpen();
	}
	
	else if (keys & KEY_Y)
	{
		CycleBrightness();
		prefs->Write();
	}
	
	else if (keys & KEY_SELECT)
	{
		mode = APP_MODE_PREFS;
		prefsSelected = 0;
		PrefsDraw();
	}
	
	else if (keys & (key.left | key.l))
	{
		// next book.
		int b = GetBookIndex(bookselected);
		if (b < bookcount-1)
		{
			b++;
			bookselected = books[b];
			if (b >= browserstart+APP_BROWSER_BUTTON_COUNT) {
				browser_nextpage();
				browser_draw();
			} else {
				browser_redraw();
			}
		}
	}

	else if (keys & (key.right | key.r))
	{
		// previous book.
		int b = GetBookIndex(bookselected);
		if (b > 0)
		{
			b--;
			bookselected = books[b];
			if(b < browserstart) {
				browser_prevpage();
				browser_draw();
			} else {
				browser_redraw();
			}	
		}
	}

	else if (keys & (KEY_START | KEY_B))
	{
#if 0
		// Only back up into the last book if it
		// wasn't closed while trying to open another one.
		if(bookcurrent && bookcurrent->GetPage())
		{
			bookcurrent->GetPage()->Draw(ts);
			mode = APP_MODE_BOOK;
			prefs->Write();
		}
#endif
	}

	else if (keysHeld() & KEY_TOUCH)
	{
		touchPosition touch;
		touchRead(&touch);
		touchPosition coord;

		// Transform point according to screen orientation.
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
				if (buttons[i]->EnclosesPoint(coord.py, coord.px))
				{
					bookselected = books[i];
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
		Book *book = books[i];
		buttons.push_back(new Button());
		buttons[i]->Init(ts);
		buttons[i]->Move(0,(i%APP_BROWSER_BUTTON_COUNT)*32);
		if (strlen(books[i]->GetTitle()))
			buttons[i]->Label(books[i]->GetTitle());
		else
			buttons[i]->Label(books[i]->GetFileName());
		if (book->GetAuthor())
			buttons[i]->SetLabel2(*(book->GetAuthor()));
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

	if (!bookselected) {
		browserstart = 0;
		bookselected = books[0];
	} else {
	browserstart = (GetBookIndex(bookselected) / APP_BROWSER_BUTTON_COUNT)
		* APP_BROWSER_BUTTON_COUNT;
	}
}

void App::browser_nextpage()
{
	if(browserstart+APP_BROWSER_BUTTON_COUNT < bookcount)
	{ 
		browserstart += APP_BROWSER_BUTTON_COUNT;
		bookselected = books[browserstart];
	}
}

void App::browser_prevpage()
{
	if(browserstart-APP_BROWSER_BUTTON_COUNT >= 0)
	{	
		browserstart -= APP_BROWSER_BUTTON_COUNT;
		bookselected = books[browserstart+APP_BROWSER_BUTTON_COUNT-1];
	}
}

void App::browser_draw(void)
{
	// save state.
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();
 	u16 *screen = ts->GetScreen();
	int style = ts->GetStyle();
	
	ts->SetScreen(ts->screenright);
	ts->SetInvert(false);
	ts->ClearScreen();
	ts->SetStyle(TEXT_STYLE_BROWSER);
	ts->SetPixelSize(PIXELSIZE);
	for (int i=browserstart;
		(i<bookcount) && (i<browserstart+APP_BROWSER_BUTTON_COUNT);
		i++)
	{
		buttons[i]->Draw(ts->screenright,books[i]==bookselected);
	}
	
	if(browserstart >= APP_BROWSER_BUTTON_COUNT)
		buttonprev.Draw(ts->screenright,false);
	if(bookcount > browserstart+APP_BROWSER_BUTTON_COUNT)
		buttonnext.Draw(ts->screenright,false);

	buttonprefs.Draw(ts->screenright,false);

	// restore state.
	ts->SetInvert(invert);
	ts->SetPixelSize(size);
	ts->SetScreen(screen);
	ts->SetStyle(style);
}

void App::browser_redraw()
{
	//! Redraw all buttons visible in the browser.
	// only call this when incrementing or decrementing the
	// selected book; otherwise use browser_draw().

	// save state.
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();
	int style = ts->GetStyle();
	
	ts->SetScreen(ts->screenright);
	ts->SetInvert(false);
	ts->SetPixelSize(PIXELSIZE);
	ts->SetStyle(TEXT_STYLE_BROWSER);
	int b = GetBookIndex(bookselected);
	buttons[b]->Draw(ts->screenright,true);
	if(b > browserstart)
		buttons[b-1]->Draw(ts->screenright,false);
	if(b < bookcount-1 &&
		(b - browserstart) < APP_BROWSER_BUTTON_COUNT-1)
		buttons[b+1]->Draw(ts->screenright,false);

	// restore state.
	ts->SetInvert(invert);
	ts->SetPixelSize(size);
	ts->SetStyle(style);
}

void App::AttemptBookOpen()
{
	if (!OpenBook()) {
		mode = APP_MODE_BOOK;
		//UpdateClock();
	} else
		browser_draw();
}

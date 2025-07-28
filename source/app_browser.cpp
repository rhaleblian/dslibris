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
#include <nds/arm9/input.h>

#include "main.h"
#include "parse.h"
#include "app.h"
#include "book.h"
#include "button.h"
#include "text.h"

#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)

void App::browser_handleevent()
{
	auto keys = keysDown();
	
	if (keys & (KEY_A | key.down))
	{
		OpenBook();
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
			if (b >= browserstart + APP_BROWSER_BUTTON_COUNT) browser_nextpage();
			browser_view_dirty = true;
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
			// TODO does not allow highlight to update
			if(b < browserstart) browser_prevpage();
			browser_view_dirty = true;
		}
	}

	else if (keys & KEY_TOUCH)
	{
		touchPosition coord = TouchRead();

		if(buttonnext.EnclosesPoint(coord.py, coord.px))
		{
			browser_nextpage();
		}
		else if(buttonprev.EnclosesPoint(coord.py, coord.px))
		{
			browser_prevpage();
		}
		else if(buttonprefs.EnclosesPoint(coord.py, coord.px))
		{
			// Move to settings view
			prefs_view_dirty = true;  // Request a redraw
			mode = APP_MODE_PREFS;
		}
		else
		{
			/// Open this book
			for(u8 i=browserstart; 
				(i<bookcount) && (i<browserstart+APP_BROWSER_BUTTON_COUNT);
				i++) {
				if (buttons[i]->EnclosesPoint(coord.py, coord.px))
				{
					bookselected = books[browserstart + i];
					OpenBook();
					break;
				}
			}
		}
	}
}

void App::browser_init(void)
{
	for (int i=0;i<bookcount;i++)
	{
		Book *book = books[i];
		buttons.push_back(new Button());
		buttons[i]->Init(ts);
		buttons[i]->Move(0,(i%APP_BROWSER_BUTTON_COUNT)*buttons[i]->GetHeight());
		const char* title = books[i]->GetTitle();
		if (title && strlen(title))
			buttons[i]->SetLabel1(std::string(title));
		else
			buttons[i]->SetLabel1(std::string(books[i]->GetFileName()));
		if (book->GetAuthor().size())
			buttons[i]->SetLabel2(book->GetAuthor());
	}

	buttonprev.Init(ts);
	buttonprev.Move(2,238);
	buttonprev.Resize(60,16);
	buttonprev.Label("prev");
	buttonnext.Init(ts);
	buttonnext.Move(130,238);
	buttonnext.Resize(60,16);
	buttonnext.Label("next");
	buttonprefs.Init(ts);
	buttonprefs.Move(66,238);
	buttonprefs.Resize(60,16);
	buttonprefs.Label("settings");

	if (!bookselected) {
		browserstart = 0;
		bookselected = books[0];
	} else {
	browserstart = (GetBookIndex(bookselected)
		/ APP_BROWSER_BUTTON_COUNT)
		* APP_BROWSER_BUTTON_COUNT;
	}
}

void App::browser_nextpage()
{
	if(browserstart+APP_BROWSER_BUTTON_COUNT < bookcount)
	{ 
		browserstart += APP_BROWSER_BUTTON_COUNT;
		bookselected = books[browserstart];
		browser_view_dirty = true;
	}
}

void App::browser_prevpage()
{
	if(browserstart-APP_BROWSER_BUTTON_COUNT >= 0)
	{	
		browserstart -= APP_BROWSER_BUTTON_COUNT;
		bookselected = books[browserstart+APP_BROWSER_BUTTON_COUNT-1];
		browser_view_dirty = true;
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

	browser_view_dirty = false;
}

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

#include "app.h"
#include "book.h"
#include "button.h"
#include "parse.h"
#include "prefs.h"
#include "text.h"

#include "browser.h"

Browser::Browser(App* a, u16* gfx)
{
	app = a;
	screen = gfx;
	bookselected = nullptr;
	offset = 0;
	maxperpage = APP_BROWSER_BUTTON_COUNT;
	bookselected = nullptr;
	offset = 0;
	// TODO compute this from object dimensions.
	maxperpage = APP_BROWSER_BUTTON_COUNT;

	int i = 0;
	buttons.clear();
	buttons.reserve(app->books.size());
	for (auto book : app->books) {
		auto button = new Button();
		button->Init(app->ts);
		if (strlen(book->GetTitle()))
			button->Label(book->GetTitle());
		else
			button->Label(book->GetFileName());
		button->Move(0, (i % maxperpage) * 32);
		buttons.push_back(button);
	}
	i++;
}

Browser::~Browser()
{
	for (u8 i=0;i<buttons.size();i++) delete buttons[i];
	buttons.clear();
}

void Browser::Init(void)
{
	// Move to the selected book.
	if (bookselected) {
		offset = (app->GetBookIndex(bookselected) / maxperpage)
			* maxperpage;
	} else {
		offset = 0;
		bookselected = app->books[0];
	}

	// menubutton[0].Init(ts);
	// menubutton[0].Move(2,238);
	// menubutton[0].Resize(60,16);
	// menubutton[0].Label("prev");
	// menubutton[2].Init(ts);
	// menubutton[2].Move(130,238);
	// menubutton[2].Resize(60,16);
	// menubutton[2].Label("next");
	// menubutton[1].Init(ts);
	// menubutton[1].Move(66,238);
	// menubutton[1].Resize(60,16);
	// menubutton[1].Label("prefs");
}

void Browser::Draw(void)
{
	//! Draw the browser screen.

	printf("%d %d\n", app->books.size(), buttons.size());
	// printf("%d %p\n", app->bg[0], app->ts->screenleft);
	// buttons[0]->Draw(screen, false);

	return;

	// Text *ts = app->ts;

	// Push state.
	// bool invert = ts->GetInvert();
	// u8 size = ts->GetPixelSize();
 	// u16* screen = ts->GetScreen();
	// int style = ts->GetStyle();

	// ts->SetScreen(screen);
	// ts->SetInvert(false);
	// ts->SetStyle(TEXT_STYLE_BROWSER);
	// ts->SetPixelSize(PIXELSIZE);

	// Book buttons.

	// u8 n = books->size();
	// for (u8 i = offset;
	// 	 i < n && (i < offset + maxperpage);
	// 	 i++)
	// {
	// 	buttons[i]->Draw(screen, (*books)[i]==bookselected);
	// }
	
	// Bottom menu buttons.

	// if(offset >= APP_BROWSER_BUTTON_COUNT)
	// 	buttonprev.Draw(ts->screenright,false);
	// if(books.size() > offset+APP_BROWSER_BUTTON_COUNT)
	// 	buttonnext.Draw(ts->screenright,false);

	// buttonprefs.Draw(ts->screenright,false);

	// Pop state.
	// ts->SetInvert(invert);
	// ts->SetPixelSize(size);
	// ts->SetScreen(screen);
	// ts->SetStyle(style);
}

void Browser::Redraw()
{
	//! Redraw changed buttons visible in the browser.
	// only call this when incrementing or decrementing the
	// selected book; otherwise use browser_draw().

	Text *ts = app->ts;

	// save state.
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();
	int style = ts->GetStyle();
	
	ts->SetScreen(ts->screenright);
	ts->SetInvert(false);
	ts->SetPixelSize(PIXELSIZE);
	ts->SetStyle(TEXT_STYLE_BROWSER);

	u8 b = app->GetBookIndex(bookselected);
	u8 bookssize = app->books.size();
	if (b < 0 || b >= bookssize) {
		b = offset;
		bookselected = app->books[b];
	}
	buttons[b]->Draw(ts->screenright,true);
	if(b > offset)
		buttons[b-1]->Draw(ts->screenright,false);
	if(b < bookssize-1 &&
		(b - offset) < APP_BROWSER_BUTTON_COUNT-1)
		buttons[b+1]->Draw(ts->screenright,false);

	// restore state.
	ts->SetInvert(invert);
	ts->SetPixelSize(size);
	ts->SetStyle(style);
}

void Browser::AdvancePage()
{
	u8 bookssize = app->books.size();
	if(offset + maxperpage < bookssize)
	{ 
		offset += maxperpage;
		bookselected = app->books[offset];
	}
}

void Browser::RetreatPage()
{
	if(offset - maxperpage >= 0)
	{	
		offset -= maxperpage;
		bookselected = app->books[offset + maxperpage-1];
	}
}

void Browser::HandleEvent()
{
	uint32_t keys = keysDown();
	auto key = app->key;
	u8 bookcount = (app->books).size();
	
	if (keys & (KEY_A | key.down))
	{
		app->OpenBook(bookselected);
	}
	
	else if (keys & KEY_Y)
	{
		app->CycleBrightness();
		app->prefs->Write();
	}
	
	else if (keys & KEY_SELECT)
	{
		app->mode = APP_MODE_PREFS;
		app->prefsSelected = 0;
		app->PrefsDraw();
	}
	
	else if (keys & (key.left | key.l))
	{
		// next book.
		u8 b = app->GetBookIndex(GetBookSelected());
		if (b < bookcount - 1)
		{
			b++;
			SetBookSelected(app->books[b]);
			if (b >= offset+APP_BROWSER_BUTTON_COUNT) {
				AdvancePage();
				Draw();
			} else {
				Redraw();
			}
		}
	}

	else if (keys & (key.right | key.r))
	{
		// previous book.
		int b = app->GetBookIndex(bookselected);
		if (b > 0)
		{
			b--;
			bookselected = app->books[b];
			if(b < offset) {
				RetreatPage();
				Draw();
			} else {
				Redraw();
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
		if(!app->ts->orientation)
		{
			coord.px = 256 - touch.px;
			coord.py = touch.py;
		} else {
			coord.px = touch.px;
			coord.py = 192 - touch.py;
		}

		if(menubutton[2].EnclosesPoint(coord.py, coord.px))
		{
			AdvancePage();
			Draw();
		}
		else if(menubutton[0].EnclosesPoint(coord.py, coord.px))
		{
			RetreatPage();
			Draw();
		}
		else if(menubutton[1].EnclosesPoint(coord.py, coord.px))
		{
			if(app->mode != APP_MODE_PREFS) {
				app->mode = APP_MODE_PREFS;
				app->prefsSelected = 0;
				app->PrefsDraw();
			} else {
				app->mode = APP_MODE_BROWSER;
				Draw();
			}
		} else {
			for(u8 i=offset; 
				(i<bookcount) &&
				(i<offset+APP_BROWSER_BUTTON_COUNT);
				i++) {
				if (buttons[i]->EnclosesPoint(coord.py, coord.px))
				{
					bookselected = app->books[i];
					Draw();
					swiWaitForVBlank();
					app->OpenBook(bookselected);
					break;
				}
			}
		}
	}	
}

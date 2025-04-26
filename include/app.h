#pragma once
/*
 Copyright (C) 2007-2009 Ray Haleblian (ray23@sourceforge.net)
 
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
#include <expat.h>
#include <list>
#include <sstream>
#include <vector>
#include <unistd.h>
#include "button.h"

class Book;
class Browser;
class Text;
class Prefs;

typedef struct {
	u16 up,down,left,right,l,r,a,b,x,y,start,select;
	uint32_t downrepeat;
} input_t;

//! \brief Main application.
//!
//!	\detail Top-level singleton class that handles application initialization,
//! interaction loop, drawing everything but text, and logging.

class App {
	public:
	App();
	~App();
	void ClearScreen(u16 *screen, u8 r, u8 g, u8 b);
	u8 GetBookIndex(Book* book);
	Text* GetTypesetter();
	u8 OpenBook(Book *book);
	void SetControls(bool flipped = false);

	//! in App.cpp
	void CycleBrightness();
	void Flip();
	u8 Init();
	u8 Init2();
	void PrintStatus(const char *msg);
	void PrintStatus(std::string msg);
	int  Run(void);
	void SetProgress(int amount);

	//! in App_Prefs.cpp
	void HandleEventInPrefs();
	void PrefsInit();
	void PrefsDraw();
	void PrefsDraw(bool redraw);
	void PrefsButton();
	void PrefsIncreasePixelSize();
	void PrefsDecreasePixelSize();
	void PrefsIncreaseParaspacing();
	void PrefsDecreaseParaspacing();
	void PrefsRefreshButtonFont();
	void PrefsRefreshButtonFontBold();
	void PrefsRefreshButtonFontItalic();
	void PrefsRefreshButtonFontSize();
	void PrefsRefreshButtonParaspacing();
	
	//! in App_Font.cpp
	void HandleEventInFont();
	void FontInit();
	void FontDeinit();
	void FontDraw(bool redraw = true);
	void FontNextPage();
	void FontPreviousPage();
	void FontButton();

	void SetBrightness(int b);
	void DrawSplashScreen();

	Text* ts;		 //! Typesetter.
	Prefs* prefs;    //! Preferences and serialization
	Browser* browser; //! Browser view.
	u8 brightness;   //! 4 levels for the Lite.
	u8 mode; 	     //! Are we in book or browser or prefs mode?
	std::string fontdir;  //! Default location to search for TTF files.
	input_t key; 	      //! Inputs are remappable to support screen flipping.
	std::vector<Button*> buttons;
	std::string bookdir;  //! search path for EPUB files.
	std::vector<Book*> books;	//! books found in {bookdir}.
	Book* bookcurrent;		//! which book is currently being read?
	
	bool reopen;     //! reopen book from last session on startup?
	bool cache;      //! save rendered glypth to a cache?

	std::vector<u16> pageindices; 	//! not used yet; will contain pagination indices for caching.
	
	// TODO move to Prefs class.
	Button prefsButtonBooks;
	Button prefsButtonFonts;
	Button prefsButtonFont;
	Button prefsButtonFontBold;
	Button prefsButtonFontItalic;
	Button prefsButtonFontSize;
	Button prefsButtonParaspacing;
	Button* prefsButtons[PREFS_BUTTON_COUNT];
	u8 prefsSelected;

	int bg[2];

	private:
	void Fatal(const char *msg);
	void HandleEventInBook();
	void HandleEventInBrowser();

	void IndexBooks();
 	void ReopenBook();
	void SortBookmarks();
	void SortBooks();

	void SetOrientation(bool flip);

	uint8_t bookmaxbuttons;  // max that fit on a screen.
	Button buttonnext;
	Button buttonprev;
	Button buttonprefs;

};

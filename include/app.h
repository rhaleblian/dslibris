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

#ifndef APP_H
#define APP_H

/*!
\mainpage

dslibris is an ebook reader for the Nintendo DS family
of handheld game consoles.

For information about prerequisites, building, and installing,
see

https://github.com/rhaleblian/dslibris/blob/master/README.md

This documentation was built by running

  make doc

in the repo root directory. The origin Git repo is located at

https://github.com/rhaleblian/dslibris

\author ray haleblian

*/


#include <list>
#include <sstream>
#include <unistd.h>
#include <vector>

#include "expat.h"

#include "book.h"
#include "button.h"
#include "prefs.h"
#include "text.h"
#include "main.h"
#include "parse.h"

#define APP_BROWSER_BUTTON_COUNT 7
#define APP_LOGFILE "dslibris.log"
#define APP_MODE_BOOK 0
#define APP_MODE_BROWSER 1
#define APP_MODE_PREFS 2
#define APP_MODE_PREFS_FONT 3
#define APP_MODE_PREFS_FONT_BOLD 4
#define APP_MODE_PREFS_FONT_ITALIC 5
#define APP_URL "http://github.com/rhaleblian/dslibris"

#define PREFS_BUTTON_COUNT 5
#define PREFS_BUTTON_FONT 2
#define PREFS_BUTTON_FONT_ITALIC 3
#define PREFS_BUTTON_FONT_BOLD 4
#define PREFS_BUTTON_FONTSIZE 1
#define PREFS_BUTTON_PARASPACING 0


//! \brief Main application.
//!
//!	\detail Top-level singleton class that handles application initialization,
//! interaction loop, drawing everything but text, and logging.

class App {
	private:
	void InitScreens();
	void SetBrightness(int b);
	void SetOrientation(bool flip);
	void WifiInit();
	bool WifiConnect();
	void Fatal(const char *msg);

	public:
	Text *ts;
	Prefs myprefs;   //?
	Prefs *prefs;    //?
	u8 brightness;   //! 4 levels for the Lite.
	u8 mode; 	     //! Are we in book or browser mode?
	string fontdir;  //! Default location to search for TTFs.
	bool console;    //! Can we print to console at the moment?
	
	//! key functions are remappable to support screen flipping.
	struct {
		u16 up,down,left,right,l,r,a,b,x,y,start,select;
		uint32 downrepeat;
	} key;
	
	vector<Button*> buttons;
	Button buttonprev, buttonnext, buttonprefs; //! Buttons on browser bottom.
	//! index into book vector denoting first book visible on library screen. 
	u8 browserstart; 
	string bookdir;  //! Search here for XHTML.
	vector<Book*> books;
	u8 bookcount;
	//! which book is currently selected in browser?
	Book* bookselected;
	//! which book is currently being read?
	Book* bookcurrent;
	//! reopen book from last session on startup?
	bool reopen;
	//! Write baked text to cache?
	bool cache;
	//! user data block passed to expat callbacks.
	parsedata_t parsedata;
	//! not used yet; will contain pagination indices for caching.
	vector<u16> pageindices;
	u8 orientation;
	u8 paraspacing, paraindent;
	
	Button prefsButtonBooks;
	Button prefsButtonFonts;
	Button prefsButtonFont;
	Button prefsButtonFontBold;
	Button prefsButtonFontItalic;
	Button prefsButtonFontSize;
	Button prefsButtonParaspacing;
	Button* prefsButtons[PREFS_BUTTON_COUNT];
	u8 prefsSelected;
	
	unsigned int fontSelected;
	unsigned int fontPage;
	vector<Button*>fontButtons;

	//BImage *image0;
	//BScreen *bscreen0;
	//BProgressBar *progressbar;

	App();
	~App();
	
	//! in App.cpp
	void CycleBrightness();
	void PrintStatus(const char *msg);
	void PrintStatus(string msg);
	void Flip();
	void SetProgress(int amount);
	void UpdateClock();

	void Log(const char*);
	void Log(const char* format, const char *msg);
	void Log(const std::string);
	void Log(const char *format, const int value);

	int  Run(void);
	bool parse_in(parsedata_t *data, context_t context);
	void parse_init(parsedata_t *data);
	context_t parse_pop(parsedata_t *data);
	void parse_error(XML_ParserStruct *ps);
	void parse_push(parsedata_t *data, context_t context);

	//! in App_Browser.cpp
	void HandleEventInBrowser();
	void browser_init(void);
	void browser_draw(void);
	void browser_nextpage(void);
	void browser_prevpage(void);
	void browser_redraw(void);
	
	//! in App_Book.cpp
	void HandleEventInBook();
	int  GetBookIndex(Book*);
	void AttemptBookOpen();
	u8   OpenBook(void);
	
	//! in App_Prefs.cpps
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
	void FontDraw();
	void FontDraw(bool redraw);
	void FontNextPage();
	void FontPreviousPage();
	void FontButton();
};

#endif


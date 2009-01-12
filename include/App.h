/*
 Copyright (C) 2007-2009 Ray Haleblian
 
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
 
 To contact the copyright holder: ray@haleblian.com
 */

#ifndef APP_H
#define APP_H

#include <nds.h>
#include <fat.h>
#include <expat.h>
#include <unistd.h>

#include "Prefs.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"

#include "main.h"
#include "parse.h"

#include <vector>
#include <list>

#define APP_BROWSER_BUTTON_COUNT 6
#define APP_LOGFILE "dslibris.log"
#define APP_MODE_BOOK 0
#define APP_MODE_BROWSER 1
#define APP_MODE_PREFS 2
#define APP_MODE_PREFS_FONT 3
#define APP_MODE_PREFS_FONT_BOLD 4
#define APP_MODE_PREFS_FONT_ITALIC 5
#define APP_URL "http://sourceforge.net/projects/ndslibris"

#define PREFS_BUTTON_COUNT 7
#define PREFS_BUTTON_BOOKS 0
#define PREFS_BUTTON_FONTS 1
#define PREFS_BUTTON_FONT 2
#define PREFS_BUTTON_FONT_ITALIC 3
#define PREFS_BUTTON_FONT_BOLD 4
#define PREFS_BUTTON_FONTSIZE 5
#define PREFS_BUTTON_PARASPACING 6


//! \brief Main application.
//!
//!	\detail Top-level singleton class that handles application initialization,
//! interaction loop, drawing everything but text, and logging.

class App {
	private:
	void Fatal(const char *msg);

	public:
	Text *ts;
	Prefs myprefs;
	Prefs *prefs;
	u16 *screenleft, *screenright, *fb;
	int bgMain, bgSplash, bgSub;
	//! level as per DS Lite.
	u8 brightness;
	//! are we in book or browser mode?
	u8 mode;
	string fontdir;
	
	//! key functions are remappable to support screen flipping.
	struct {
		u16 up,down,left,right,l,r,a,b,x,y,start,select;
	} key;
	
	vector<Button*> buttons;
	Button buttonprev, buttonnext, buttonprefs;
	//! index into book vector denoting first book visible on library screen. 
	u8 browserstart;
	string bookdir;
	vector<Book*> books;
	u8 bookcount;
	//! which book is currently selected in browser? -1=none.
	u8 bookselected;
	//! which book is currently being read? -1=none.
	s8 bookcurrent;
	//! reopen book from last session on startup?
	bool reopen;
	//! user data block passed to expat callbacks.
	parsedata_t parsedata;
	//! pointer to array of page data for current book.
	page_t *pages;
	u8 *pagebuf;
	u16 pagecount;
	u16 pagecurrent;
	//! not used yet; will contain pagination indices for caching.
	vector<u16> pageindices;
	char *filebuf,*msg;
	u8 marginleft, marginright, margintop, marginbottom;
	u8 linespacing;
	u8 orientation;
	u8 paraspacing, paraindent;
	bool bookBold;
	bool bookItalic;
	
	Button prefsButtonBooks;
	Button prefsButtonFonts;
	Button prefsButtonFont;
	Button prefsButtonFontBold;
	Button prefsButtonFontItalic;
	Button prefsButtonFontSize;
	Button prefsButtonParaspacing;
	Button* prefsButtons[PREFS_BUTTON_COUNT];
	u8 prefsSelected;
	
	u8 fontSelected;
	Button* fontButtons;
	vector<Text*> fontTs;
	u8 fontPage;

	App();
	~App();
	int  Run(void);
	
	void CycleBrightness();
	void Log(const char*);
	void Log(std::string);
	void PrintStatus(const char *msg);
	void PrintStatus(string msg);
	void Flip();
	void SetProgress(int amount);
	void UpdateClock();

	void HandleEventInBrowser();
	void browser_init(void);
	void browser_draw(void);
	void browser_nextpage(void);
	void browser_prevpage(void);
	void browser_redraw(void);
	void AttemptBookOpen();
	
	u8   OpenBook(void);
	void HandleEventInBook();
	void page_init(page_t *page);
	void page_draw(page_t *page);
	void page_drawmargins(void);
	u8   page_getjustifyspacing(page_t *page, u16 i);
	
	void HandleEventInPrefs();
	void PrefsInit();
	void PrefsDraw();
	void PrefsDraw(bool redraw);
	void PrefsButton();
	void PrefsIncreasePixelSize();
	void PrefsDecreasePixelSize();
	void PrefsIncreaseParaspacing();
	void PrefsDecreaseParaspacing();
	void PrefsRefreshButtonBooks();
	void PrefsRefreshButtonFonts();
	void PrefsRefreshButtonFont();
	void PrefsRefreshButtonFontBold();
	void PrefsRefreshButtonFontItalic();
	void PrefsRefreshButtonFontSize();
	void PrefsRefreshButtonParaspacing();
	
	void HandleEventInFont();
	void FontInit();
	void FontDeinit();
	void FontDraw();
	void FontDraw(bool redraw);
	void FontNextPage();
	void FontPreviousPage();
	void FontButton();

	void parse_printerror(XML_ParserStruct *ps);
	bool parse_in(parsedata_t *data, context_t context);
	void parse_init(parsedata_t *data);
	bool parse_pagefeed(parsedata_t *data, page_t *page);
	context_t parse_pop(parsedata_t *data);
	void parse_push(parsedata_t *data, context_t context);

	//BImage *image0;
	//BScreen *bscreen0;
	//BProgressBar *progressbar;
};

#endif

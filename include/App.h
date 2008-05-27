#ifndef APP_H
#define APP_H

#include <nds.h>
#include <expat.h>
#include <unistd.h>

#include "Book.h"
#include "Button.h"
#include "Prefs.h"
#include "Text.h"

#include "main.h"
#include "parse.h"

#include <vector>

#define APP_BROWSER_BUTTON_COUNT 6
#define APP_LOGFILE "dslibris.log"
#define APP_MODE_BOOK 0
#define APP_MODE_BROWSER 1
#define APP_MODE_PREFS 2
#define APP_MODE_PREFS_FONT 3
#define APP_MODE_PREFS_FONT_BOLD 4
#define APP_MODE_PREFS_FONT_ITALIC 5
#define APP_URL "http://ndslibris.sourceforge.net"

#define PREFS_BUTTON_COUNT 7
#define PREFS_BUTTON_BOOKS 0
#define PREFS_BUTTON_FONTS 1
#define PREFS_BUTTON_FONT 2
#define PREFS_BUTTON_FONT_ITALIC 3
#define PREFS_BUTTON_FONT_BOLD 4
#define PREFS_BUTTON_FONTSIZE 5
#define PREFS_BUTTON_PARASPACING 6

class App {
	private:
	void InitScreens();
	void WifiInit();
	bool WifiConnect();

	public:
	Text *ts;
	class Prefs *prefs;
	u16 *screen0, *screen1, *screenleft, *screenright, *fb;
	u16 screenwidth, screenheight, pagewidth, pageheight;
	u8 brightness;
	u8 mode;
	string fontdir;
	
	Button *buttons;
	Button buttonprev, buttonnext, buttonprefs;
	u8 browserstart;
	string bookdir;
	Book *books;
	u8 bookcount;
	u8 bookselected;
	s8 bookcurrent;
	
	u8 reopen;
	parsedata_t parsedata;
	page_t *pages;
	u8 *pagebuf;
	u16 pagecount;
	u16 pagecurrent;
	vector<u16> pageindices;
	char *filebuf;
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
	
	void CycleBrightness();
	void PrintStatus(const char *msg);
	void PrintStatus(string msg);
	
	void Log(const char*);
	void Log(std::string);
	void Log(int x);
	void Log(const char* format, const char *msg);
	int  Run(void);

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
};

#endif


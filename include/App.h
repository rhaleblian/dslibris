#ifndef APP_H
#define APP_H

#include <nds.h>
#include <expat.h>

#include "Book.h"
#include "Button.h"
#include "Prefs.h"
#include "Text.h"

#include "main.h"
#include "parse.h"

#define APP_VERSION "1.1.141"
#define APP_URL "http://ndslibris.sourceforge.net"
#define APP_LOGFILE "dslibris.log"
#define APP_MODE_BOOK 0
#define APP_MODE_BROWSER 1

class App {
	public:
	Text *ts;
	class Prefs *prefs;
	u16 *screen0, *screen1, *fb;
	Button *buttons;
	Button buttonprev, buttonnext;
	u8 browserstart;
	Book *books;
	u8 bookcount;
	u8 bookcurrent;
	parsedata_t parsedata;
	page_t *pages;
	u8 *pagebuf;
	u16 pagecount;
	u16 pagecurrent;
	u8 brightness;
	char *filebuf;
	u8 mode;

	App();
	~App();

	void HandleEventInBrowser();
	void HandleEventInBook();
	void Log(const char*);
	void Log(std::string);
	u8   OpenBook(void);
	int  Run(void);

	void browser_init(void);
	void browser_draw(void);
	void browser_nextpage(void);
	void browser_prevpage(void);
	void browser_redraw(void);
	
	void page_init(page_t *page);
	void page_draw(page_t *page);
	void page_drawmargins(void);
	u8   page_getjustifyspacing(page_t *page, u16 i);

	void parse_printerror(XML_ParserStruct *ps);
	bool parse_in(parsedata_t *data, context_t context);
	void parse_init(parsedata_t *data);
	bool parse_pagefeed(parsedata_t *data, page_t *page);
	context_t parse_pop(parsedata_t *data);
	void parse_push(parsedata_t *data, context_t context);

//	bool prefs_read(XML_Parser p);
//	bool prefs_write(void);
};

#endif


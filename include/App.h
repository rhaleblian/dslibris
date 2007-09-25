#ifndef _app_h
#define _app_h

#include <nds.h>
#include <expat.h>
#include "main.h"
#include "parse.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"

#define APP_VERSION "0.2.4"
#define APP_URL "http://rhaleblian.wordpress.com"

class App {
	public:
	Text *ts;
	u16 *screen0, *screen1, *fb;
	Button *buttons;
	Book *books;
	u8 bookcount;
	u8 bookcurrent;
	parsedata_t parsedata;
	page_t *pages;	
	u8 *pagebuf;
	u16 pagecount;
	u16 pagecurrent;
	App();
	void browser_init(void);
	void browser_draw(void);
	void page_init(page_t *page);
	void page_draw(page_t *page);
	void page_drawmargins(void);
	u8   page_getjustifyspacing(page_t *page, u16 i);
	void parse_printerror(XML_ParserStruct *ps);
	bool parse_in(parsedata_t *data, context_t context);
	bool parse_pagefeed(parsedata_t *data, page_t *page);
	context_t parse_pop(parsedata_t *data);
	void parse_push(parsedata_t *data, context_t context);
	void parse_init(parsedata_t *data);
	int  main(void);
	void screen_clear(u16 *screen, u8 r, u8 g, u8 b);
	void splash_draw(void);
	bool prefs_read(XML_Parser p);
	bool prefs_write(void);
};
#endif

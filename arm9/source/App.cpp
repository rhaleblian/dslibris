#include <errno.h>
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

App::App()
{	
	ts = NULL;
	buttons = new Button[MAXBOOKS];
	browserstart = 0;
	pages = new page_t[MAXPAGES];
	pagebuf = new u8[PAGEBUFSIZE];
	pagecount = 0;
	pagecurrent = 0;
	pagewidth = PAGE_WIDTH;
	pageheight = PAGE_HEIGHT;

	books = new Book[MAXBOOKS];
	bookcount = 0;
	bookcurrent = 0;
	reopen = 0;
	mode = APP_MODE_BROWSER;
	filebuf = (char*)malloc(sizeof(char) * BUFSIZE);

	screenwidth = SCREEN_WIDTH;
	screenheight = SCREEN_HEIGHT;
	marginleft = MARGINLEFT;
	margintop = MARGINTOP;
	marginright = MARGINRIGHT;
	marginbottom = MARGINBOTTOM;
	linespacing = LINESPACING;
	orientation = 0;
	paraspacing = 1;
	paraindent = 0;
	brightness = 0;

	prefs = new Prefs(this);
}

App::~App()
{
	free(filebuf);
	delete books;
	delete buttons;
	delete pages;
	delete prefs;
}

int App::Run(void)
{
	char filebuf[BUFSIZE];
	char msg[128];

	powerSET(POWER_LCD|POWER_2D_A|POWER_2D_B);
	defaultExceptionHandler();  // guru meditation!

	// set up ARM7 interrupts and IPC.

	irqInit();
	irqEnable(IRQ_VBLANK);
	irqEnable(IRQ_VCOUNT);
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;

	// go to the lowest brightness setting.

	NDSX_SetBrightness_Next();

	// get the filesystem going first
	// so we can write a log.

	if (!fatInitDefault()) exit(-11);

	Log("\n");
	Log("info : dslibris starting up.\n");

	ts = new Text();
	ts->app = this;
	XML_Parser p = XML_ParserCreate(NULL);
	if (!p)
	{
		ts->PrintString("fatal: parser creation failed.\n");
		Log("fatal: parser creation failed.\n");
		exit(-6);
	}
	XML_SetUnknownEncodingHandler(p,unknown_hndl,NULL);
	parse_init(&parsedata);

	// construct library.

	char dirname[32];
	strcpy(dirname,BOOKDIR);
	sprintf(msg,"info : scanning '%s' for books.\n",dirname);
	Log(msg);

	DIR_ITER *dp = diropen(dirname);
	if (!dp)
	{
		Log("fatal: no book directory.\n");
		swiWaitForVBlank();
		exit(-3);
	}

	char filename[64];
	while (bookcount < MAXBOOKS)
	{
		int rc = dirnext(dp, filename, NULL);
		if(rc) break;

		char *c;
		for (c=filename;c!=filename+strlen(filename) && *c!='.';c++);
		if (!stricmp(".xht",c) || !stricmp(".xhtml",c))
		{
			Book *book = &(books[bookcount]);
			book->SetFolderName(dirname);
			book->SetFileName(filename);
			
			sprintf(msg,"info : indexing book '%s'.\n", book->GetFileName());
			Log(msg);

			u8 rc = book->Index(filebuf);
			if(rc == 255) {
				sprintf(msg, "fatal: cannot index book '%s'.\n",
					book->GetFileName());
				Log(msg);
				exit(-4);
			}
			if(rc == 254) {
				sprintf(msg, "fatal: cannot make book parser.");
				exit(-8);
			}
			sprintf(msg, "info : book title '%s'.\n",book->GetTitle());
			Log(msg);
			bookcount++;
		}
	}
	dirclose(dp);
	swiWaitForVBlank();

	// read preferences.

	Log("info : reading prefs.\n");
	
   	if(!prefs->Read(p))
	{
		Log("warn : could not open preferences.\n");
	}

	// init typesetter.

	Log("info : starting typesetter.\n");

	int err = ts->Init();
   	if (err) {
		Log("fatal: starting typesetter failed.\n");
		exit(-2);
	}

	// initialize screens.

	for(int b=0; b<brightness; b++)
		NDSX_SetBrightness_Next();

#ifdef GRIT

	vramSetMainBanks(
		VRAM_A_MAIN_BG_0x06000000,
		VRAM_B_MAIN_BG_0x06020000,
		VRAM_C_SUB_BG_0x06200000,
		VRAM_D_LCD
	);			
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);

	BG3_CR = BG_BMP16_256x256 | BG_BMP_BASE(0);
	SUB_BG3_CR = BG_BMP16_256x256 | BG_BMP_BASE(0);

	BG3_XDX = 1 << 8;
	BG3_XDY = 0;
	BG3_YDX = 0;
	BG3_YDY = 1 << 8;
	BG3_CX = 0;
	BG3_CY = 0;
	SUB_BG3_XDX = 1 << 8;
	SUB_BG3_XDY = 0;
	SUB_BG3_YDX = 0;
	SUB_BG3_YDY = 1 << 8;
	SUB_BG3_CX = 0;
	SUB_BG3_CY = 0;

	pagewidth = screenwidth;
	pageheight = screenheight;

#else

	BACKGROUND.control[3] = BG_BMP16_256x256 | BG_BMP_BASE(0);
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	BACKGROUND_SUB.control[3] = BG_BMP16_256x256 | BG_BMP_BASE(0);
	videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);

	u16 angle;
	if(orientation) {
		angle = 128;
		BG3_CX = 192 << 8;
		BG3_CY = 0 << 8;
		SUB_BG3_CX = 192 << 8;
		SUB_BG3_CY = 0 << 8;
		screen1 = (u16*)BG_BMP_RAM_SUB(0);
		screen0 = (u16*)BG_BMP_RAM(0);
	}
	else
	{
		angle = 384;
		BG3_CX = 0 << 8;
		BG3_CY = 256 << 8;
		SUB_BG3_CX = 0 << 8;
		SUB_BG3_CY = 256 << 8;
		screen0 = (u16*)BG_BMP_RAM_SUB(0);
		screen1 = (u16*)BG_BMP_RAM(0);
	}

	s16 s = SIN[angle & 0x1FF] >> 4;
	s16 c = COS[angle & 0x1FF] >> 4;
	BG3_XDX = c;
	BG3_XDY = -s;
	BG3_YDX = s;
	BG3_YDY = c;
	SUB_BG3_XDX = c;
	SUB_BG3_XDY = -s;
	SUB_BG3_YDX = s;
	SUB_BG3_YDY = c;

#endif

	screen1 = (u16*)BG_BMP_RAM(0);
	screen0 = (u16*)BG_BMP_RAM_SUB(0);

	if(orientation) ts->PrintSplash(screen1); 
	else ts->PrintSplash(screen0);

	browser_init();

	if(reopen)
	{
		if(!OpenBook()) mode = APP_MODE_BOOK;
	}
	else
	{
		mode = APP_MODE_BROWSER;
		browser_draw();
	}
	swiWaitForVBlank();

	// start event loop.

	bool poll = true;
	while (poll)
	{
		scanKeys();

		if(mode == APP_MODE_BROWSER)
			HandleEventInBrowser();
		else
			HandleEventInBook();
		
		swiWaitForVBlank();
	}

	if(p) XML_ParserFree(p);
	exit(0);
}

void App::HandleEventInBrowser()
{
	if (keysDown() & KEY_A)
	{
		// parse the selected book.
		if(!OpenBook()) {
			mode = APP_MODE_BOOK;
			reopen = 1;
		}
		else browser_draw();
	}

	else if (keysDown() & KEY_B)
	{
		// return to current book.
		mode = APP_MODE_BOOK;
		page_draw(&pages[pagecurrent]);
	}

	else if (keysDown() & (KEY_LEFT|KEY_L))
	{
		if (bookcurrent < bookcount-1)
		{
			if (bookcurrent == browserstart+6) {
				browser_nextpage();
				browser_draw();
			} else {
				bookcurrent++;
				browser_redraw();
			}
		}
	}

	else if (keysDown() & (KEY_RIGHT|KEY_R))
	{
		if (bookcurrent > 0)
		{
			if(bookcurrent == browserstart) {
				browser_prevpage();
				browser_draw();
			} else {
				bookcurrent--;
				browser_redraw();
			}				
		}
	}

	else if (keysDown() & KEY_SELECT)
	{
		mode = APP_MODE_BOOK;
		page_draw(&(pages[pagecurrent]));
	}

	else if (keysDown() & KEY_TOUCH)
	{
		touchPosition touch = touchReadXY();
		touchPosition coord;
		u8 regionprev[2], regionnext[2];
		regionprev[0] = 0;						
		regionprev[1] = 16;
		regionnext[0] = 240;
		regionnext[1] = 255;	

		if(!orientation)
		{
			coord.px = 256 - touch.px;
			coord.py = 192 - touch.py;
		} else {
			coord.px = touch.px;
			coord.py = touch.py;
		}

		if(coord.px > regionnext[0]
			&& coord.px < regionnext[1])
		{
			browser_nextpage();
			browser_draw();
		}
		else if(coord.px > regionprev[0]
			&& coord.px < regionprev[1])
		{
			browser_prevpage();
			browser_draw();
		} else {
			for(u8 i=browserstart;
				(i<bookcount) && (i<browserstart+7);
				i++) {
				if (buttons[i].EnclosesPoint(coord.py, coord.px))
				{
					bookcurrent = i;
					browser_draw();
					swiWaitForVBlank();
					if(!OpenBook()) mode = APP_MODE_BOOK;
					else browser_draw();
					break;
				}
			}
		}
	}	
}

void App::HandleEventInBook()
{
	if (keysDown() & (KEY_A|KEY_DOWN|KEY_R))
	{
		if (pagecurrent < pagecount)
		{
			pagecurrent++;
			page_draw(&pages[pagecurrent]);
			books[bookcurrent].SetPosition(pagecurrent);
			prefs->Write();
		}
	}

	else if (keysDown() & (KEY_B|KEY_UP|KEY_L))
	{
		if (pagecurrent > 0)
		{
			pagecurrent--;
			page_draw(&pages[pagecurrent]);
			books[bookcurrent].SetPosition(pagecurrent);
			prefs->Write();
		}
	}

	else if (keysDown() & KEY_X)
	{
		ts->SetInvert(!ts->GetInvert());
		page_draw(&pages[pagecurrent]);
		prefs->Write();
	}

	else if (keysDown() & KEY_Y)
	{
		NDSX_SetBrightness_Next();
		brightness++;
		brightness = brightness > 3 ? 0 : brightness;
		prefs->Write();
	}

	else if (keysDown() & KEY_SELECT)
	{
		prefs->Write();
		mode = APP_MODE_BROWSER;			
		if(orientation) ts->PrintSplash(screen1);
		else ts->PrintSplash(screen0);
		browser_draw();
	}

	else if (keysDown() & KEY_TOUCH)
	{
		touchPosition touch = touchReadXY();
		if (touch.py < 96)
		{
			if (pagecurrent > 0) pagecurrent--;
		}
		else
		{
			if (pagecurrent < pagecount) pagecurrent++;
		}
		page_draw(&pages[pagecurrent]);
	}

	// clock - only display when reading a book
	time_t tt = time(NULL);
	struct tm *tms = gmtime((const time_t *)&tt);
	char tmsg[6];
	sprintf(tmsg, "%02d:%02d", tms->tm_hour, tms->tm_min);
	u8 offset = marginleft;
	ts->SetScreen(screen0);
	ts->ClearRect(offset, 240, offset+30, 255);
	ts->SetPen(offset,250);
	ts->PrintString(tmsg);
}

u8 App::OpenBook(void)
{
	char msg[16];
	strcpy(msg,"[opening...]");
	ts->SetScreen(screen1);
	ts->ClearScreen(screen1,0,0,0);
	ts->SetPen(marginleft,PAGE_HEIGHT/2);
	bool invert = ts->GetInvert();
	ts->SetInvert(true);
	ts->PrintString(msg);
	ts->SetInvert(invert);
	swiWaitForVBlank();
	pagecount = 0;
	pagecurrent = 0;
	page_init(&pages[pagecurrent]);
	ts->ClearCache();
	if (!books[bookcurrent].Parse(filebuf))
	{
		pagecurrent = books[bookcurrent].GetPosition();
		page_draw(&(pages[pagecurrent]));
		prefs->Write();
		return 0;
	}
	else return 255;
}

void App::browser_init(void)
{
	u8 i;
	for (i=0;i<bookcount;i++)
	{
		buttons[i].Init(ts);
		buttons[i].Move(2,((i%7+1)*32)-16);
		if (strlen(books[i].GetTitle()))
			buttons[i].Label(books[i].GetTitle());
		else
			buttons[i].Label(books[i].GetFileName());
	}
	buttonprev.Init(ts);
	buttonprev.Move(2,0);
	buttonprev.Resize(188,16);
	buttonprev.Label("^");
	buttonnext.Init(ts);
	buttonnext.Move(2,240);
	buttonnext.Resize(188,16);
	buttonnext.Label("v");
	browserstart = (bookcurrent / 7) * 7;
}

void App::browser_nextpage()
{
	if(browserstart+7 < bookcount)
	{ 
		browserstart += 7;
		bookcurrent = browserstart;
	}
}

void App::browser_prevpage()
{
	if(browserstart-7 >= 0)
	{
		browserstart -= 7;
		bookcurrent = browserstart+6;
	}
}

void App::browser_draw(void)
{
	// save state.
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();

#ifndef GRIT
	u16* screen;
	if(orientation) screen = screen0;
	else screen = screen1;
 
	ts->SetScreen(screen);
	ts->ClearScreen(screen,0,0,0);
	ts->SetPixelSize(12);
	for (int i=browserstart;(i<bookcount) && (i<browserstart+7);i++)
	{
		if (i==bookcurrent)
			buttons[i].Draw(screen,true);
		else
			buttons[i].Draw(screen,false);
	}

	if(browserstart > 6) 
		buttonprev.Draw(screen,false);
	if(bookcount > browserstart+7)
		buttonnext.Draw(screen,false);
#endif

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

#ifndef GRIT

	u16 *screen;
	if(orientation) screen = screen0;
	else screen = screen1;

	ts->SetScreen(screen);
	buttons[bookcurrent].Draw(screen,true);
	if(bookcurrent > browserstart)
		buttons[bookcurrent-1].Draw(screen,false);
	if(bookcurrent < bookcount-1 && bookcurrent - browserstart < 6)
		buttons[bookcurrent+1].Draw(screen,false);

#endif

	// restore state.
	ts->SetInvert(invert);
	ts->SetPixelSize(size);
}

void App::page_init(page_t *page)
{
	page->length = 0;
	page->buf = NULL;
}

u8 App::page_getjustifyspacing(page_t *page, u16 i)
{
	/** full justification. get line advance, count spaces,
	    and insert more space in spaces to reach margin.
	    returns amount of space to add per-character. **/

	u8 spaces = 0;
	u8 advance = 0;
	u8 j,k;

	/* walk through leading spaces */
	for (j=i;j<page->length && page->buf[j]==' ';j++);

	/* find the end of line */
	for (j=i;j<page->length && page->buf[j]!='\n';j++)
	{
		u16 c = page->buf[j];
		advance += ts->GetAdvance(c);

		if (page->buf[j] == ' ') spaces++;
	}

	/* walk back through trailing spaces */
	for (k=j;k>0 && page->buf[k]==' ';k--) spaces--;

	if (spaces)
		return((u8)((float)((PAGE_WIDTH-marginright-marginleft) - advance)
		            / (float)spaces));
	else return(0);
}


void App::parse_printerror(XML_Parser p)
{
	u16 *screen = ts->GetScreen();
	u16 x,y;
	ts->GetPen(x,y);

	char msg[256];
	sprintf(msg,"line %d, col %d: %s\n",
		(int)XML_GetCurrentLineNumber(p),
		(int)XML_GetCurrentColumnNumber(p),
		XML_ErrorString(XML_GetErrorCode(p)));
	Log(msg);

	ts->SetScreen(screen0);
	ts->InitPen();
	ts->ClearScreen();
	ts->PrintString(msg);

	ts->SetScreen(screen);
	ts->SetPen(x,y);
}

void App::parse_init(parsedata_t *data)
{
	data->stacksize = 0;
	data->book = NULL;
	data->page = NULL;
	data->pen.x = marginleft;
	data->pen.y = margintop;
}

void App::parse_push(parsedata_t *data, context_t context)
{
	data->stack[data->stacksize++] = context;
}

context_t App::parse_pop(parsedata_t *data)
{
	if (data->stacksize) data->stacksize--;
	return data->stack[data->stacksize];
}

bool App::parse_in(parsedata_t *data, context_t context)
{
	u8 i;
	for (i=0;i<data->stacksize;i++)
	{
		if (data->stack[i] == context) return true;
	}
	return false;
}

bool App::parse_pagefeed(parsedata_t *data, page_t *page)
{
	// called when we are at the end of one of the facing pages.

	bool pagedone = false;
	
	if (fb == screen1)
	{
		// we left the right page, save chars into this page.

		if (!page->buf)
		{
			page->buf = new u8[page->length];
			if (!page->buf)
			{
				ts->PrintString("[out of memory]\n");
				exit(-7);
			}
		}
		memcpy(page->buf,pagebuf,page->length * sizeof(u8));
		fb = screen0;
		pagedone = true;
	}
	else
	{
		fb = screen1;
		pagedone = false;
	}
	data->pen.x = marginleft;
	data->pen.y = margintop + ts->GetHeight();
	return pagedone;
}

void App::page_draw(page_t *page)
{
	ts->SetScreen(screen1);
	ts->ClearScreen();
	ts->SetScreen(screen0);
	ts->ClearScreen();
	ts->InitPen();
	bool linebegan = false;

	u16 i=0;
	while (i<page->length)
	{
		u32 c = page->buf[i];
		if (c == '\n')
		{
			// line break, page breaking if necessary

			i++;

			if (ts->GetPenY() + ts->GetHeight() + linespacing 
				> PAGE_HEIGHT - marginbottom)
			{
				if(ts->GetScreen() == screen0) {
					ts->SetScreen(screen1);
					ts->InitPen();
					linebegan = false;
				}
				else break;
			}
			else if (linebegan) ts->PrintNewLine();
		}
		else
		{
			if (c > 127) i+=ts->GetCharCode((char*)&(page->buf[i]),&c);
			else i++;
			ts->PrintChar(c);
			linebegan = true;
		}
	}

	// page number

	char msg[8];
	strcpy(msg,"");
	if(pagecurrent == 0) 
		sprintf((char*)msg,"[ %d >",pagecurrent+1);
	else if(pagecurrent == pagecount)
		sprintf((char*)msg,"< %d ]",pagecurrent+1);
	else
		sprintf((char*)msg,"< %d >",pagecurrent+1);
	ts->SetScreen(screen1);
	u8 offset = (u8)((PAGE_WIDTH-marginleft-marginright-(ts->GetAdvance(40)*7))
		* (pagecurrent / (float)pagecount));
	ts->SetPen(marginleft+offset,250);
	ts->PrintString(msg);
}

void App::Log(const char *msg)
{
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,msg);
	fclose(logfile);
}

void App::Log(std::string msg)
{
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,msg.c_str());
	fclose(logfile);
}

void App::Log(int x)
{
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,"%d",x);		
	fclose(logfile);
}

void App::Log(const char *format, const char *msg)
{
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,format,msg);
	fclose(logfile);	
}


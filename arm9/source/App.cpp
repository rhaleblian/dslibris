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
	
	books = new Book[MAXBOOKS];
	bookcount = 0;
	bookcurrent = 0;
	mode = APP_MODE_BOOK;
	filebuf = (char*)malloc(sizeof(char) * BUFSIZE);
}

App::~App()
{
	free(filebuf);
}

int App::Run(void)
{
	char filebuf[BUFSIZE];
	char msg[128];

	powerSET(POWER_LCD|POWER_2D_A|POWER_2D_B);
	defaultExceptionHandler();  // guru meditation!

	printf("hello world.\n");

	// set up ARM7 interrupts and IPC.

	irqInit();
	irqEnable(IRQ_VBLANK);
	irqEnable(IRQ_VCOUNT);
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;

	// go to the lowest brightness setting.

	NDSX_SetBrightness_Next();
	brightness = 0;

	// initialize screens.
	// clockwise rotation for both screens

	s16 s = SIN[-128 & 0x1FF] >> 4;
	s16 c = COS[-128 & 0x1FF] >> 4;

	BACKGROUND.control[3] = BG_BMP16_256x256 | BG_BMP_BASE(0);
	BG3_XDX = c;
	BG3_XDY = -s;
	BG3_YDX = s;
	BG3_YDY = c;
	BG3_CX = 0 << 8;
	BG3_CY = 256 << 8;
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	screen1 = (u16*)BG_BMP_RAM(0);
	BACKGROUND_SUB.control[3] = BG_BMP16_256x256 | BG_BMP_BASE(0);
	SUB_BG3_XDX = c;
	SUB_BG3_XDY = -s;
	SUB_BG3_YDX = s;
	SUB_BG3_YDY = c;
	SUB_BG3_CX = 0 << 8;
	SUB_BG3_CY = 256 << 8;
	videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	screen0 = (u16*)BG_BMP_RAM_SUB(0);

	if (!fatInitDefault())
	{
		printf("fatal: mounting filesystem failed\n"); 
		exit(-1);
	}

	Log("\ninfo : dslibris starting up\n");

	ts = new Text();
	if (ts->InitDefault())
//	if (ts->InitWithCacheManager())
	{
		Log("fatal: starting typesetter failed\n");
		printf("fatal: starting typesetter failed\n");
		exit(-2);
	}

	screen_splash();

	// assemble library by indexing all XHTML files
	// in the application directory.

	char dirname[32];
	strcpy(dirname,BOOKDIR);
	sprintf(msg,"info : scanning %s for books\n",dirname);
	Log(msg);

	DIR_ITER *dp = diropen(dirname);
	if (!dp)
	{
		ts->PrintString("fatal: no book directory\n");
		Log("fatal: no book directory\n");
		swiWaitForVBlank();
		exit(-3);
	}

	char filename[64];
	while (bookcount < MAXBOOKS)
	{
		int rc = dirnext(dp, filename, NULL);
		if(rc) break;

		sprintf(msg,"info : found %s\n", filename);
		Log(msg);

		char *c;
		for (c=filename;c!=filename+strlen(filename) && *c!='.';c++);
		if (!stricmp(".xht",c) || !stricmp(".xhtml",c))
		{
			Book *book = &(books[bookcount]);
			book->SetFolderName(dirname);
			book->SetFileName(filename);
			
			sprintf(msg,"info : indexing %s (in folder %s)\n",
				book->GetFileName(),
				book->GetFolderName());
			Log(msg);

			u8 rc = book->Index(filebuf);
			if(rc == 255) {
				ts->PrintString("cant open");
				exit(-4);
			}
			if(rc == 254) {
				ts->PrintString("cant make parser");
				exit(-8);
			}
			bookcount++;
		}
	}
	dirclose(dp);
	swiWaitForVBlank();


	// restore the last book and page we were reading.
	// TODO bookmark character, not page

	XML_Parser p = XML_ParserCreate(NULL);
	if (!p)
	{
		ts->PrintString("fatal: parser creation failed\n");
		Log("fatal: parser creation failed\n");
		exit(-6);
	}
	XML_SetUnknownEncodingHandler(p,unknown_hndl,NULL);
	parse_init(&parsedata);

	bookcurrent = 127;
	mode = APP_MODE_BROWSER;
	prefs_read(p);
	if(bookcurrent < 127)
	{
		browser_init();
		if(!OpenBook()) mode = APP_MODE_BOOK;
	}
	else
	{
		bookcurrent = 0;
		browser_init();
		browser_draw();
	}
	swiWaitForVBlank();

	// start event loop.

	touchPosition touch;
	bool poll = true;
	while (poll)
	{
		scanKeys();
			
		if (keysDown() & KEY_Y)
		{
			NDSX_SetBrightness_Next();
			brightness++;
			brightness = brightness > 3 ? 0 : brightness;
		}

		if (keysDown() & KEY_TOUCH)
		{
			touch = touchReadXY();
			
/*
			if (mode == APP_MODE_BROWSER)
			{
				if(touch.px < 16) {
					browser_nextpage();
					browser_draw();
				} else if(touch.px > 240) {
					browser_prevpage();
					browser_draw();
				} else {
					for(u8 i=browserstart;
						i<bookcount && i<browserstart+7;
						i++) {
						if (buttons[i].EnclosesPoint(touch.px, touch.py))
						{
							bookcurrent = i;
							browser_redraw();
							swiWaitForVBlank();
							if(!OpenBook()) mode = APP_MODE_BOOK;
							break;
						}
					}	
				}
			}
			else
			{
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
*/
		}

		if (mode == APP_MODE_BROWSER)
		{
			if (keysDown() & KEY_A)
			{
				// parse the selected book.

				if(!OpenBook()) mode = APP_MODE_BOOK;
				else browser_draw(); 
			/*	
				pagecount = 0;
				pagecurrent = 0;
				page_t *page = &(pages[pagecurrent]);
				page_init(page);

				screen_clear(screen1,0,0,0);
				ts->SetScreen(screen1);
				ts->SetPen(PAGE_WIDTH/2
					- ts->GetStringWidth("[opening...]")/2,
					PAGE_HEIGHT/2);
				bool invert = ts->GetInvert();
				ts->SetInvert(true);
				ts->PrintString("[opening...]");
				ts->SetInvert(invert);
				ts->ClearCache();
				if (!books[bookcurrent].Parse(filebuf))
				{
					pagecurrent = books[bookcurrent].GetPosition();
					page = &(pages[pagecurrent]);
					page_draw(page);
					prefs_write();
					mode = APP_MODE_BOOK;
				}
				else
				{
					browser_draw();
				}
*/
			}

			if (keysDown() & KEY_B)
			{
				mode = APP_MODE_BOOK;
				page_draw(&pages[pagecurrent]);
			}

			if (keysDown() & (KEY_LEFT|KEY_L))
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

			if (keysDown() & (KEY_RIGHT|KEY_R))
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

			if (keysDown() & KEY_SELECT)
			{
				mode = APP_MODE_BOOK;
				page_draw(&(pages[pagecurrent]));
			}
		}
		else
		{
			if (keysDown() & (KEY_A|KEY_DOWN|KEY_R))
			{
				if (pagecurrent < pagecount)
				{
					pagecurrent++;
					page_draw(&pages[pagecurrent]);
					books[bookcurrent].SetPosition(pagecurrent);
					prefs_write();
				}
			}

			if (keysDown() & (KEY_B|KEY_UP|KEY_L))
			{
				if (pagecurrent > 0)
				{
					pagecurrent--;
					page_draw(&pages[pagecurrent]);
					books[bookcurrent].SetPosition(pagecurrent);
					prefs_write();
				}
			}

			if (keysDown() & KEY_X)
			{
				ts->SetInvert(!ts->GetInvert());
				page_draw(&pages[pagecurrent]);
			}

			if (keysDown() & KEY_SELECT)
			{
				prefs_write();
				mode = APP_MODE_BROWSER;			
				screen_splash();
				browser_draw();
			}

		}
		swiWaitForVBlank();
	}

	if(p) XML_ParserFree(p);
	exit(0);
}

u8 App::OpenBook(void)
{
	char msg[16];
	strcpy(msg,"[opening...]");
	ts->SetScreen(screen1);
	screen_clear(screen1,0,0,0);
	ts->SetPen(MARGINLEFT,PAGE_HEIGHT/2);
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
		prefs_write();
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
		buttons[i].Move(0,((i%7+1)*32)-16);
		if (strlen(books[i].GetTitle()))
			buttons[i].Label(books[i].GetTitle());
		else
			buttons[i].Label(books[i].GetFileName());
	}
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
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();
	Button buttonprev, buttonnext;

	ts->SetScreen(screen1);
	screen_clear(screen1,0,0,0);
	ts->SetPixelSize(12);
	for (int i=browserstart;(i<bookcount) && (i<browserstart+7);i++)
	{
		if (i==bookcurrent)
			buttons[i].Draw(screen1,true);
		else
			buttons[i].Draw(screen1,false);
	}

/*	buttonprev.Init(ts);
	buttonprev.Move(0,0);
	buttonprev.Resize(192,16);
	buttonprev.Label("^");
	buttonnext.Init(ts);
	buttonnext.Move(0,240);
	buttonnext.Resize(192,16);
	buttonnext.Label("v");
	buttonprev.Draw(screen1,false);
	buttonnext.Draw(screen1,false);
*/

	ts->SetInvert(invert);
	ts->SetPixelSize(size);
}

void App::browser_redraw()
{
	buttons[bookcurrent].Draw(screen1,true);
	if(bookcurrent > browserstart)
		buttons[bookcurrent-1].Draw(screen1,false);
	if(bookcurrent < bookcount-1 && bookcurrent - browserstart < 6)
		buttons[bookcurrent+1].Draw(screen1,false);
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
		return((u8)((float)((PAGE_WIDTH-MARGINRIGHT-MARGINLEFT) - advance)
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
	data->pen.x = MARGINLEFT;
	data->pen.y = MARGINTOP;
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
	data->pen.x = MARGINLEFT;
	data->pen.y = MARGINTOP + ts->GetHeight();
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

			if (ts->GetPenY() + ts->GetHeight() + LINESPACING 
				> PAGE_HEIGHT - MARGINBOTTOM)
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
	u8 offset = (u8)((PAGE_WIDTH-MARGINLEFT-MARGINRIGHT-(ts->GetAdvance(40)*7))
		* (pagecurrent / (float)pagecount));
	ts->SetPen(MARGINLEFT+offset,250);
	ts->PrintString(msg);
}

bool App::prefs_read(XML_Parser p)
{
	FILE *fp = fopen(PREFSPATH,"r");
	if (!fp) return false;

	XML_ParserReset(p, NULL);
	XML_SetStartElementHandler(p, prefs_start_hndl);
	XML_SetUserData(p, (void *)books);
	while (true)
	{
		void *buff = XML_GetBuffer(p, 64);
		int bytes_read = fread(buff, sizeof(char), 64, fp);
		XML_ParseBuffer(p, bytes_read, bytes_read == 0);
		if (bytes_read == 0) break;
	}
	fclose(fp);
	return true;
}

bool App::prefs_write(void)
{
	FILE* fp = fopen(PREFSPATH,"w+");
	if(!fp) return false;
	
	fprintf(fp, "<dslibris>\n");
	fprintf(fp, "\t<screen brightness=\"%d\" invert=\"%d\">\n",
		brightness, ts->GetInvert());
	fprintf(fp, "\t<font size=\"%d\" />\n", ts->GetPixelSize());
	fprintf(fp, "\t<book file=\"%s\" />\n", books[bookcurrent].GetFileName());
	for(u8 i=0;i<bookcount; i++)
	{
		fprintf(fp, "\t<bookmark file=\"%s\" page=\"%d\" />\n",
	        books[i].GetFileName(), books[i].GetPosition()+1);
	}
	fprintf(fp, "</dslibris>\n");
	fclose(fp);

	return true;
}


void App::screen_clear(u16 *screen, u8 r, u8 g, u8 b)
{
	for (int i=0;i<PAGE_HEIGHT*PAGE_HEIGHT;i++)
		screen[i] = RGB15(r,g,b) | BIT(15);
}

void App::screen_splash(void)
{
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();

	ts->SetInvert(true);
	ts->SetScreen(screen0);
	screen_clear(screen0,0,0,0);
	ts->SetPen(SPLASH_LEFT,SPLASH_TOP);
	ts->SetPixelSize(36);
	ts->PrintString("dslibris");
	ts->SetPixelSize(10);
	ts->SetPen(SPLASH_LEFT,ts->GetPenY()+ts->GetHeight());
	ts->PrintString("an ebook reader");
	ts->SetPen(SPLASH_LEFT,ts->GetPenY()+ts->GetHeight());
	ts->PrintString("for Nintendo DS");
	ts->SetPen(SPLASH_LEFT,ts->GetPenY()+ts->GetHeight());
	ts->PrintString(APP_VERSION);
	ts->PrintNewLine();
	ts->SetPen(SPLASH_LEFT,ts->GetPenY()+ts->GetHeight());

	ts->SetPixelSize(size);
	ts->SetInvert(invert);
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


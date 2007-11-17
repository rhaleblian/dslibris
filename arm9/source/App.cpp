#include <fat.h>
#include <dswifi9.h>
#include <nds/registers_alt.h>
#include <nds/reload.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <errno.h>
#include <expat.h>
#include "types.h"
#include "ndsx_brightness.h"
#include "App.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"
#include "main.h"
#include "parse.h"
#include "wifi.h"

inline void spin(void)
{
	while (true) swiWaitForVBlank();
}

void swiWaitForKeys() 
{
  asm("mov r0, #1");
  asm("mov r1, #4096");
  asm("swi #262144");
}

void consoleOK(bool ok)
{
	printf("[");

	if(ok)
	{
		BG_PALETTE_SUB[255] = RGB15(15,31,15);
		printf(" OK ");
	}
	else
	{
		BG_PALETTE_SUB[255] = RGB15(31,15,15);
		printf("FAIL");
	}
			
	BG_PALETTE_SUB[255] = RGB15(24,24,24);
	printf("]\n");
}

int min(int x, int y)
{
	if (y < x) return y;
	else return x;
}

App::App()
{
	ts = NULL;
	buttons = new Button[MAXBOOKS];
	
	pages = new page_t[MAXPAGES];
	pagebuf = new u8[PAGEBUFSIZE];
	pagecount = 0;
	pagecurrent = 0;
	
	books = new Book[MAXBOOKS];
	bookcount = 0;
	bookcurrent = 0;
}

int App::main(void)
{
	bool browseractive = false;
	char filebuf[BUFSIZE];

	powerSET(POWER_LCD|POWER_2D_A|POWER_2D_B);
	defaultExceptionHandler();  /** guru meditation! **/

	irqInit();
	irqEnable(IRQ_VBLANK);
	irqEnable(IRQ_VCOUNT);
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;

	// this ought to be the lowest brightness setting.
	NDSX_SetBrightness_Next();			

	/** bring up the startup console.
	    sub bg 0 will be used to print text. **/
	// TODO don't display console; log to a file.
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	vramSetBankC(VRAM_C_SUB_BG);
	SUB_BG0_CR = BG_MAP_BASE(31);
	{
		u32 i;
		for (i=0;i<255;i++)
			BG_PALETTE_SUB[i] = RGB15(0,0,0);
		BG_PALETTE_SUB[255] = RGB15(04,24,04);
		consoleInitDefault(
			(u16*)SCREEN_BASE_BLOCK_SUB(31),
			(u16*)CHAR_BASE_BLOCK_SUB(0), 16);
	}
	printf(" Starting console...     ");
	consoleOK(true);

	printf(" Mounting filesystem...  ");
	if (!fatInitDefault())
	{
		consoleOK(false);
		spin();
	} else consoleOK(true);

	printf(" Starting typesetter...  ");
	ts = new Text();
	if (ts->InitDefault())
	{
		consoleOK(false);
		spin();
	} else consoleOK(true);

	swiWaitForVBlank();
	/** assemble library by indexing all
		XHTML/XML files in the current directory.
		**/
	printf(" Scanning for books...   ");
	bookcount = 0;
	bookcurrent = 0;
	char filename[64];
	DIR_ITER *dp = diropen(BOOKPATH);
	if (!dp)
	{
		consoleOK(false);
		spin();
	}
	while (!dirnext(dp, filename, NULL) && (bookcount != MAXBOOKS))
	{
		char *c;
		for (c=filename;c!=filename+strlen(filename) && *c!='.';c++);
		if (!stricmp(".xht",c) || !stricmp(".xhtml",c))
		{
			Book *book = &(books[bookcount]);
			book->SetFilename(filename);
			book->Index(filebuf);
			bookcount++;
		}
		if (!stricmp(".htm",c) || !stricmp(".html",c))
		{
			/** NYI - this doesn't work yet **/
			Book *book = &(books[bookcount]);
			book->SetFilename(filename);
			book->IndexHTML(filebuf);
			bookcount++;
		}
	}
	dirclose(dp);
	if (!bookcount)
	{
		consoleOK(false);
		spin();
	}
	consoleOK(true);
	swiWaitForVBlank();
	browser_init();

	printf(" Creating XML parser...  ");
	XML_Parser p = XML_ParserCreate(NULL);
	if (!p)
	{
		consoleOK(false);
		spin();
	}
	XML_SetUnknownEncodingHandler(p,unknown_hndl,NULL);
	parse_init(&parsedata);
	consoleOK(true);
	swiWaitForVBlank();

	/** initialize screens. **/

	/** clockwise rotation for both screens **/
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

	browser_init();
	splash_draw();
	
	/** restore the last book and page we were reading. **/
	/** TODO bookmark character, not page **/

	bookcurrent = 127;
	if(prefs_read(p) && bookcurrent < 127)
	{
		ts->SetScreen(screen1);
		ts->SetPen(MARGINLEFT+40,PAGE_HEIGHT/2);
		bool invert = ts->GetInvert();
		ts->SetInvert(true);
		ts->PrintString("[paginating...]");
		ts->SetInvert(invert);
		swiWaitForVBlank();

		pagecount = 0;
		pagecurrent = 0;
		page_init(&(pages[pagecurrent]));
		ts->Cache();
		if(!books[bookcurrent].Parse(filebuf))
		{
			pagecurrent = books[bookcurrent].GetPosition();
			page_draw(&(pages[pagecurrent]));
			browseractive = false;
		} else browseractive = true;
	} else {
		bookcurrent = 0;
		browseractive = true;
	}
	
	if(browseractive)
	{
		browser_draw();
	}
	swiWaitForVBlank();

	touchPosition touch;

	bool poll = true;
	while (poll)
	{
		scanKeys();

		if (browseractive)
		{
			if (keysDown() & KEY_TOUCH)
			{
				touch = touchReadXY();
				fb[touch.px + touch.py * 256] = rand();
				if (pagecurrent < pagecount)
				{
					pagecurrent++;
					page_draw(&pages[pagecurrent]);
				}
			}

			if (keysDown() & KEY_A)
			{
				/** parse the selected book. **/

				pagecount = 0;
				pagecurrent = 0;
				page_t *page = &(pages[pagecurrent]);
				page_init(page);

				screen_clear(screen1,0,0,0);
				ts->SetScreen(screen1);
				ts->SetPen(MARGINLEFT+40,PAGE_HEIGHT/2);
				bool invert = ts->GetInvert();
				ts->SetInvert(true);
				ts->PrintString("[paginating...]");
				ts->SetInvert(invert);
				ts->Cache();
				if (!books[bookcurrent].Parse(filebuf))
				{
					pagecurrent = books[bookcurrent].GetPosition();
					page = &(pages[pagecurrent]);
					page_draw(page);
					prefs_write();
					browseractive = false;
				}
				else
				{
					splash_draw();
					browser_draw();
				}
			}

			if (keysDown() & KEY_B)
			{
				browseractive = false;
				page_draw(&pages[pagecurrent]);
			}

			if (keysDown() & (KEY_LEFT|KEY_L))
			{
				if (bookcurrent < bookcount-1)
				{
					bookcurrent++;
					browser_draw();
				}
			}

			if (keysDown() & (KEY_RIGHT|KEY_R))
			{
				if (bookcurrent > 0)
				{
					bookcurrent--;
					browser_draw();
				}
			}

			if (keysDown() & KEY_SELECT)
			{
				browseractive = false;
				ts->Cache();
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
			
			if (keysDown() & KEY_Y)
			{
				NDSX_SetBrightness_Next();			
			}

			if (keysDown() & KEY_SELECT)
			{
				prefs_write();
				browseractive = true;
				splash_draw();
				browser_draw();
			}
/*
			if(keysDown() & KEY_START)
			{
				poll = false;
			}
*/
		}
//		swiWaitForKeys();
		swiWaitForVBlank();
	}

	if(p) XML_ParserFree(p);
	exit(0);
}

void App::browser_init(void)
{
	u8 i;
	for (i=0;i<bookcount;i++)
	{
//		buttons[i] = new Button();
		buttons[i].Init(ts);
		buttons[i].Move(0,(i+1)*32);
		if (strlen(books[i].GetTitle()))
			buttons[i].Label(books[i].GetTitle());
		else
			buttons[i].Label(books[i].GetFilename());
	}	
}

void App::browser_draw(void)
{
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();

	ts->SetScreen(screen1);
	ts->SetPixelSize(12);
	for (int i=0;i<bookcount;i++)
	{
		if (i==bookcurrent)
			buttons[i].Draw(screen1,true);
		else
			buttons[i].Draw(screen1,false);
	}

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
		advance += ts->Advance(c);

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
	char msg[128];
	sprintf(msg,"expat: [%s]\n",XML_ErrorString(XML_GetErrorCode(p)));
	ts->PrintString(msg);
	sprintf(msg,"expat: [%d:%d] : %d\n",
	        (int)XML_GetCurrentLineNumber(p),
	        (int)XML_GetCurrentColumnNumber(p),
	        (int)XML_GetCurrentByteIndex(p));
	ts->PrintString(msg);
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
	int pagedone;

	/** we are at the end of one of the facing pages. **/
	if (fb == screen1)
	{

		/** we left the right page, save chars into this page. **/
		if (!page->buf)
		{
			page->buf = new u8[page->length];
			if (!page->buf)
			{
				ts->PrintString("[out of memory]\n");
				spin();
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
		u16 c = page->buf[i];
		if (c == '\n')
		{
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
			if (c > 127) i+=ts->GetUCS((char*)&(page->buf[i]),&c);
			else i++;
			ts->PrintChar(c);
			linebegan = true;
		}
	}

	// page number
	ts->SetScreen(screen1);
	u8 offset = (int)(170.0 * (pagecurrent / (float)pagecount));
	ts->SetPen(MARGINLEFT+offset,250);
	char msg[8];
	sprintf((char*)msg,"[%d]",pagecurrent+1);
	ts->PrintString(msg);
}

bool App::prefs_read(XML_Parser p)
{
	FILE *fp = fopen("/dslibris/dslibris.xml","r");
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
	FILE* fp = fopen("/dslibris/dslibris.xml","w+");
	if(!fp) return false;
	
	fprintf(fp, "<dslibris>\n");
	fprintf(fp, "\t<font size=\"%d\" />\n", ts->GetPixelSize());
	fprintf(fp, "\t<book file=\"%s\" />\n", books[bookcurrent].GetFilename());
	for(u8 i=0;i<bookcount; i++)
	{
		fprintf(fp, "\t<bookmark file=\"%s\" page=\"%d\" />\n",
	        books[i].GetFilename(), books[i].GetPosition()+1);
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

#define SPLASH_LEFT (MARGINLEFT+28)
#define SPLASH_TOP (MARGINTOP+96)

void App::splash_draw(void)
{
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();

	ts->SetInvert(false);
	ts->SetScreen(screen0);
	screen_clear(screen0,31,31,31);
/*	
	for(int i=1;i<256;i+=2)
	{
		memset(screen0+(i*256),RGB15(28,28,28)|BIT(15),512);
	}
*/
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
	char msg[16];
	sprintf(msg,"%d books\n", bookcount);
	ts->PrintString(msg);

	ts->SetScreen(screen1);
	screen_clear(screen1,0,0,0);
/*
	for(int i=1;i<32;i+=2)
	{
		memset(screen1+i*256,RGB15(4,4,4)|BIT(15),512);
	}
	for(int i=bookcount*32+1;i<256;i+=2)
	{
		memset(screen1+i*256,RGB15(4,4,4)|BIT(15),512);
	}
*/
	ts->SetPen(MARGINLEFT+100, MARGINTOP+12);
	ts->SetPixelSize(20);
	ts->SetInvert(true);
	ts->PrintString("library");

	ts->SetPixelSize(size);
	ts->SetInvert(invert);
}

#include <errno.h>
#include <stdlib.h>
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

//#define NOLOG

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

	fontdir = string(FONTDIR);
	bookdir = string(BOOKDIR);
	books = new Book[MAXBOOKS];
	bookcount = 0;
	bookselected = 0;
	bookcurrent = -1;
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
	brightness = 1;

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

	NDSX_SetBrightness_0();

	// get the filesystem going first
	// so we can write a log.

	if (!fatInitDefault()) exit(-11);

	Log("\n");
	Log("info : dslibris starting up.\n");

	ts = new Text();
	ts->app = this;
	ts->SetFontFile(FONTFILEPATH, 0);
	ts->SetFontBoldFile(FONTBOLDFILEPATH, 0);
	ts->SetFontItalicFile(FONTITALICFILEPATH, 0);
	XML_Parser p = XML_ParserCreate(NULL);
	if (!p)
	{
		ts->PrintString("fatal: parser creation failed.\n");
		Log("fatal: parser creation failed.\n");
		exit(-6);
	}
	XML_SetUnknownEncodingHandler(p,unknown_hndl,NULL);
	parse_init(&parsedata);

	// read preferences (to load bookdir)
	Log("info : reading prefs.\n");
   	if(!prefs->Read(p))
	{
		Log("warn : could not open preferences.\n");
	}
	
	// construct library.

	sprintf(msg,"info : scanning '%s' for books.\n",bookdir.c_str());
	Log(msg);
	
	DIR_ITER *dp = diropen(bookdir.c_str());
	if (!dp)
	{
		Log("fatal: no book directory.\n");
		swiWaitForVBlank();
		exit(-3);
	}

	char filename[MAXPATHLEN];
	while (bookcount < MAXBOOKS)
	{
		int rc = dirnext(dp, filename, NULL);
		if(rc)
			break;

		char *c;
		for (c=filename;c!=filename+strlen(filename) && *c!='.';c++);
		if (!stricmp(".xht",c) || !stricmp(".xhtml",c))
		{
			Book *book = &(books[bookcount]);
			book->SetFolderName(bookdir.c_str());
			
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
			else if(rc == 254) {
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
	
	// Sort bookmarks for each book
	for(u8 i = 0; i < bookcount; i++)
	{
		books[bookcount].GetBookmarks()->sort();
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

	
	PrefsInit();
	browser_init();

	if(reopen && !OpenBook())
	{
		mode = APP_MODE_BOOK;
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
		else if (mode == APP_MODE_BOOK)
			HandleEventInBook();
		else if (mode == APP_MODE_PREFS)
			HandleEventInPrefs();
		else if (mode == APP_MODE_PREFS_FONT || mode == APP_MODE_PREFS_FONT_BOLD || mode == APP_MODE_PREFS_FONT_ITALIC)
			HandleEventInFont();
		
		swiWaitForVBlank();
	}

	if(p)
		XML_ParserFree(p);
	exit(0);
}

void App::CycleBrightness()
{
	NDSX_SetBrightness_Next();
	brightness++;
	brightness = brightness % 4;
	prefs->Write();
}

void App::Log(const char *msg)
{
#ifdef NOLOG
	return;
#endif
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,msg);
	fclose(logfile);
}

void App::Log(std::string msg)
{
#ifdef NOLOG
	return;
#endif
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,msg.c_str());
	fclose(logfile);
}

void App::Log(int x)
{
#ifdef NOLOG
	return;
#endif
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,"%d",x);		
	fclose(logfile);
}

void App::Log(const char *format, const char *msg)
{
#ifdef NOLOG
	return;
#endif
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,format,msg);
	fclose(logfile);	
}



#include "App.h"

#include <errno.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <algorithm>   // for std::sort
#include <fat.h>
#include <nds/registers_alt.h>
#include <nds/reload.h>

#include "ndsx_brightness.h"
#include "types.h"
#include "main.h"
#include "parse.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"
#include "version.h"

// less-than operator to compare books by title
static bool book_title_lessthan(Book* a, Book* b) {
    return strcasecmp(a->GetTitle(),b->GetTitle()) < 0;
}

App::App()
{	
	fontdir = string(FONTDIR);
	bookdir = string(BOOKDIR);
	bookcount = 0;
	bookselected = NULL;
	bookcurrent = NULL;
	reopen = true;
	mode = APP_MODE_BROWSER;
	browserstart = 0;

	console = false;
	orientation = false;
	paraspacing = 1;
	paraindent = 0;
	brightness = 1;
	
	key.down = KEY_DOWN;
	key.up = KEY_UP;
	key.left = KEY_LEFT;
	key.right = KEY_RIGHT;
	key.start = KEY_START;
	key.select = KEY_SELECT;
	key.l = KEY_L;
	key.r = KEY_R;
	key.a = KEY_A;
	key.b = KEY_B;
	key.x = KEY_X;
	key.y = KEY_Y;
	
	prefs = &myprefs;
	prefs->app = this;

	ts = new Text();
	ts->app = this;
}

App::~App()
{
	delete prefs;
	delete ts;
	vector<Book*>::iterator it;
	for(it=books.begin();it!=books.end();it++)
		delete *it;
	books.clear();
}

int App::Run(void)
{
	char msg[128];
	defaultExceptionHandler();
	
	// ARM7 initialization.
	
	powerON(POWER_ALL);
//	powerSET(POWER_LCD|POWER_2D_A|POWER_2D_B);
	irqInit();
	irqEnable(IRQ_VBLANK);
	//irqEnable(IRQ_VCOUNT);
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	
	// Get a console going.
	
	SetBrightness(0);	
	videoSetMode(0);
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	vramSetBankC(VRAM_C_SUB_BG);
	SUB_BG0_CR = BG_MAP_BASE(31);
	BG_PALETTE_SUB[255] = RGB15(15,31,15);
	consoleInitDefault(
		(u16*)SCREEN_BASE_BLOCK_SUB(31),
		(u16*)CHAR_BASE_BLOCK_SUB(0),16);
	iprintf("$ dslibris\n");
	
	// Start up FAT on this media.
	
	if(!fatInitDefault())
	{
		iprintf("fatal: no filesystem.\n");
		iprintf("DLDI patch?\n");
		while(true) swiWaitForVBlank();
	} else iprintf("progr: filesystem mounted.\n");

	Log("----\nprogr: App starting up.\n");
	console = true;

	// Start up typesetter.
	// FIXME can this be cleaner?
	
	ts->SetFontFile(FONTFILEPATH, TEXT_STYLE_NORMAL);
	ts->SetFontFile(FONTBOLDFILEPATH, TEXT_STYLE_BOLD);
	ts->SetFontFile(FONTITALICFILEPATH, TEXT_STYLE_ITALIC);
	ts->SetFontFile(FONTBROWSERFILEPATH, TEXT_STYLE_BROWSER);
	ts->SetFontFile(FONTSPLASHFILEPATH, TEXT_STYLE_SPLASH);
	int err = ts->Init();
	switch(err)
	{
		case 0:
		sprintf(msg, "progr: typesetter started.\n");
		break;
		case 1:
		sprintf(msg, "fatal: no FreeType (%d).\n", err);
		break;
		case 2:
		sprintf(msg, "fatal: font not found (%d).\n",err);
		break;
		default:
		sprintf(msg, "fatal: unknown typesetting error (%d).\n",err);
	}
	Log(msg);
	if(err) while(1) swiWaitForVBlank();
	
	// Read preferences, pass 1, to look up bookdir.
	
   	if (int err = prefs->Read())
	{
		sprintf(msg,"warn : can't read prefs (%d).\n",err);
		Log(msg);
		if(err == 255) {
			Log("info : writing new prefs.\n");
			prefs->Write();
		} 
	} else 
		Log("progr: read prefs, bookdir is '%s'.\n",bookdir.c_str());
	
	SetBrightness(brightness);
	
	// Look in the book directory and construct library.

	sprintf(msg,"info : scanning '%s' for books.\n",bookdir.c_str());
	Log(msg);
	
	DIR_ITER *dp = diropen(bookdir.c_str());
	if (!dp)
	{
		sprintf(msg,"fatal: No book directory \'%s\'.\n",
			bookdir.c_str());
		Log(msg);
	} else {
		sprintf(msg,"progr: scanning '%s'.\n",bookdir.c_str());
		Log(msg);
	}
	
	char filename[MAXPATHLEN];
	while(!dirnext(dp, filename, NULL))
	{
		sprintf(msg,"info : file: %s\n", filename);
		Log(msg);
		char *c;
		for (c=filename;c!=filename+strlen(filename) && *c!='.';c++);
		if (!stricmp(".xht",c) || !stricmp(".xhtml",c))
		{
			Book *book = new Book();
			book->SetFolderName(bookdir.c_str());
			book->SetFileName(filename);
			books.push_back(book);
			bookcount++;
			
			Log("info : indexing book '%s'.\n", book->GetFileName());

			u8 rc = book->Index();
			if (rc)
			{
				sprintf(msg,"warn : indexer failed (%d).\n",
					rc);
			}
			else
			{
				sprintf(msg, "info : indexed title '%s'.\n",
					book->GetTitle());
			}
			Log(msg);
		}
	}
	dirclose(dp);
	sprintf(msg,"progr: %d books indexed.\n",bookcount);
	Log(msg);

	if(bookcurrent)
	{
		sprintf(msg,"info : currentbook = %s.\n",bookcurrent->GetTitle());
		Log(msg);
	}
	swiWaitForVBlank();
	
	// Read preferences, pass 2, to bind preferences to books.
	
   	if(int parseerror = prefs->Read())
	{
		sprintf(msg,"warn : can't read prefs (%d).\n",parseerror);
		Log(msg);
	}
	else Log("info : read prefs (books).\n");

	// Sort bookmarks for each book.
	for(u8 i = 0; i < bookcount; i++)
	{
		books[i]->GetBookmarks()->sort();
	}

	vector<Book*>::iterator it;
	for(it=books.begin(); it<books.end(); it++)
	{
		Log("info : title '%s'\n",(*it)->GetTitle());
		sprintf(msg, "info : position %d\n",
			(*it)->GetPosition());
		Log(msg);
	}
	
	// Set up preferences editing screen.
	PrefsInit();
	
	// Set up library browser screen.
	std::sort(books.begin(),books.end(),&book_title_lessthan);
	browser_init();

	Log("progr: browsers populated.\n");

	// Bring up displays.
	
	console = false;
	InitScreens();
	if(orientation) lcdSwap();
	if (prefs->swapshoulder)
	{
		int tmp = key.l;
		key.l = key.r;
		key.r = tmp;
	}

//	BImage *topImg = new BImage(256, 192, (u8*)BG_BMP_RAM(0));	
//	BScreen* topScreen = new BScreen(topImg);
//	BGUI::get()->addScreen(topScreen);
	
	mode = APP_MODE_BROWSER;
	ts->PrintSplash(ts->screenleft);
	browser_draw();

	Log("progr: browser displayed.\n");

	// Resume reading from the last session.
	
	if(reopen && bookcurrent)
	{
		int openerr = OpenBook();
		if(openerr)
			Log("warn : could not reopen previous book.\n");
		else
		{
			Log("info : reopened previous book.\n");
			mode = APP_MODE_BOOK;
		}
	}
	else Log("info : not reopening previous book.\n");

	swiWaitForVBlank();

	// Start polling event loop.
	// FIXME use interrupt driven event handling.
	
	keysSetRepeat(60,2);
	while (true)
	{
		scanKeys();

		if(mode == APP_MODE_BROWSER)
			HandleEventInBrowser();
		else if (mode == APP_MODE_BOOK)
			HandleEventInBook();
		else if (mode == APP_MODE_PREFS)
			HandleEventInPrefs();
		else if (mode == APP_MODE_PREFS_FONT 
			|| mode == APP_MODE_PREFS_FONT_BOLD
			|| mode == APP_MODE_PREFS_FONT_ITALIC)
			HandleEventInFont();

		swiWaitForVBlank();
	}

	exit(0);
}

void App::SetBrightness(int b)
{
	if(b<0) brightness = 0;
	else if(b>3) brightness = 3;
	else brightness = b;
	if(brightness == 1) NDSX_SetBrightness_1();
	else if(brightness == 2) NDSX_SetBrightness_2();
	else if(brightness == 3) NDSX_SetBrightness_3();
	else if(brightness == 0) NDSX_SetBrightness_0();
}

void App::CycleBrightness()
{
	brightness++;
	brightness = brightness % 4;
//	BGUI::get()->switchBacklight(true,true);
//	BGUI::get()->setBacklightBrightness(brightness);
	SetBrightness(brightness);
}

void App::UpdateClock()
{
	u16 *screen = ts->GetScreen();

	char tmsg[8];
	ts->SetScreen(ts->screenleft);
	sprintf(tmsg, "%02d:%02d", IPC->time.rtc.hours, IPC->time.rtc.minutes);
	u8 offset = ts->margin.left;
	ts->ClearRect(offset, 240, offset+30, 255);
	ts->SetPen(offset,250);
	ts->PrintString(tmsg);

	ts->SetScreen(screen);
}

void App::SetOrientation(bool flip)
{
   	u16 angle;
	if(flip) {
		angle = 128;
		BG3_CX = 192 << 8;
		BG3_CY = 0 << 8;
		SUB_BG3_CX = 192 << 8;
		SUB_BG3_CY = 0 << 8;
		ts->screenleft = (u16*)BG_BMP_RAM_SUB(0);
		ts->screenright = (u16*)BG_BMP_RAM(0);
		orientation = true;
		key.down = KEY_UP;
		key.up = KEY_DOWN;
		key.left = KEY_RIGHT;
		key.right = KEY_LEFT;
		key.l = KEY_R;
		key.r = KEY_L;
	}
	else
	{
		angle = 384;
		BG3_CX = 0 << 8;
		BG3_CY = 256 << 8;
		SUB_BG3_CX = 0 << 8;
		SUB_BG3_CY = 256 << 8;
		ts->screenright = (u16*)BG_BMP_RAM_SUB(0);
		ts->screenleft = (u16*)BG_BMP_RAM(0);
		orientation = false;
		key.down = KEY_DOWN;
		key.up = KEY_UP;
		key.left = KEY_LEFT;
		key.right = KEY_RIGHT;
		key.l = KEY_L;
		key.r = KEY_R;
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
}

void App::Log(const char *msg)
{
	Log("%s",msg);
}

void App::Log(std::string msg)
{
	Log("%s",msg.c_str());
}

void App::Log(const char *format, const char *msg)
{
	if(console)
	{
		char s[128];
		sprintf(s,format,msg);
		iprintf(s);
	}
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,format,msg);
	fclose(logfile);
}

void App::InitScreens() {
//	NDSX_SetBrightness_0();
//	for(int b=0; b<brightness; b++)
//		NDSX_SetBrightness_Next();

	BACKGROUND.control[3] = BG_BMP16_256x256 | BG_BMP_BASE(0);
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	BACKGROUND_SUB.control[3] = BG_BMP16_256x256 | BG_BMP_BASE(0);
	videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	ts->SetScreen(ts->screenleft);
	ts->ClearScreen();
	ts->SetScreen(ts->screenright);
	ts->ClearScreen();
	SetOrientation(orientation);
#if 0
	u16 angle;
	if(orientation) {
		angle = 128;
		BG3_CX = 192 << 8;
		BG3_CY = 0 << 8;
		SUB_BG3_CX = 192 << 8;
		SUB_BG3_CY = 0 << 8;
		ts->screenright = (u16*)BG_BMP_RAM_SUB(0);
		ts->screenleft = (u16*)BG_BMP_RAM(0);
	}
	else
	{
		angle = 384;
		BG3_CX = 0 << 8;
		BG3_CY = 256 << 8;
		SUB_BG3_CX = 0 << 8;
		SUB_BG3_CY = 256 << 8;
		ts->screenleft = (u16*)BG_BMP_RAM_SUB(0);
		ts->screenright = (u16*)BG_BMP_RAM(0);
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
}

void App::Fatal(const char *msg)
{
	Log(msg);
	while(1) swiWaitForVBlank();
}


void App::PrintStatus(const char *msg) {
	bool invert = ts->GetInvert();
	u16* screen = ts->GetScreen();
	u8 pixelsize = ts->GetPixelSize();
	const int top = 240;
	ts->SetPixelSize(11);
	ts->SetScreen(ts->screenleft);
	ts->SetInvert(false);
	// TODO why does this crash on hw?
	ts->ClearRect(0,top,ts->display.width,ts->display.height);
	ts->SetPen(10,top+10);
	ts->PrintString(msg);

	ts->SetPixelSize(pixelsize);
	ts->SetScreen(screen);
	ts->SetInvert(invert);
}


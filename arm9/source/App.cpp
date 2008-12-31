//! Top-level class that handles application initialization and logging.
//  See main.cpp for entry point.

#include "App.h"

#include <errno.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <DSGUI/BGUI.h>

#include "version.h"
#include "main.h"
#include "parse.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"

App::App()
{	
	ts = NULL;
	browserstart = 0;
	pages = new page_t[MAXPAGES];
	pagebuf = new u8[PAGEBUFSIZE];
	pagecount = 0;
	pagecurrent = 0;

	fontdir = string(FONTDIR);
	bookdir = string(BOOKDIR);
	bookcount = 0;
	bookselected = 0;
	bookcurrent = -1;
	reopen = false;
	mode = APP_MODE_BROWSER;
	filebuf = (char*)malloc(sizeof(char) * BUFSIZE);
	msg = (char*)malloc(sizeof(char) * 128);

	screenleft = (u16*)BG_BMP_RAM(0);
	screenright = (u16*)BG_BMP_RAM_SUB(0);
	marginleft = MARGINLEFT;
	margintop = MARGINTOP;
	marginright = MARGINRIGHT;
	marginbottom = MARGINBOTTOM;
	linespacing = LINESPACING;
	orientation = 0;
	paraspacing = 1;
	paraindent = 0;
	brightness = 1;

//	prefs = new Prefs(this);
	prefs = &myprefs;
	prefs->app = this;

	ts = new Text();
	ts->app = this;
}

App::~App()
{
	free(filebuf);
	free(msg);
	delete pages;
	delete prefs;
	delete ts;
}

int App::Run(void)
{
	Log("\n");
	Log("info : dslibris starting up.\n");

	// init typesetter.

	ts->SetFontFile(FONTFILEPATH, TEXT_STYLE_NORMAL);
	ts->SetFontFile(FONTBOLDFILEPATH, TEXT_STYLE_BOLD);
	ts->SetFontFile(FONTITALICFILEPATH, TEXT_STYLE_ITALIC);
	ts->SetFontFile(FONTBROWSERFILEPATH, TEXT_STYLE_BROWSER);
	ts->SetFontFile(FONTSPLASHFILEPATH, TEXT_STYLE_SPLASH);
	int err = ts->Init();
   	if (err == 1) {
		sprintf(msg, "fatal: no FreeType (%d).\n", err);
		Log(msg);
	}
	else if (err == 2)
	{
		sprintf(msg, "fatal: font not found (%d).\n",err);
		Log(msg);
	}
	else if (err != 0)
	{
		sprintf(msg, "fatal: unknown typesetting error (%d).\n",err);
		Log(msg);
	}
	else Log("info : typesetter started.\n");
	PrintStatus(VERSION);

	XML_Parser p = XML_ParserCreate(NULL);
	if (!p)
	{
		Log("fatal: no parser.\n");
	}
	Log("progr: created parser.\n");

	XML_SetUnknownEncodingHandler(p,unknown_hndl,NULL);
	parse_init(&parsedata);

	// read preferences (to load bookdir)

   	if (int parseerror = prefs->Read())
	{
		sprintf(msg,"warn : can't read prefs (%d).\n",parseerror);
		Log(msg);
	} else 
		Log("progr: read prefs.\n");
	
	// construct library.

	DIR_ITER *dp = diropen(bookdir.c_str());
	if (!dp)
	{
		sprintf(msg,"fatal: No book directory \'%s\'.\n",
			bookdir.c_str());
		Log(msg);
	} else {
		sprintf(msg,"progr: scanning '%s'.\n",bookdir.c_str());
		Log(msg);

		char filename[MAXPATHLEN];
		while(!dirnext(dp, filename, NULL))
		{
			iprintf(filename);
			char *c;
			for (c=filename;c!=filename+strlen(filename) && *c!='.';c++);
			if (!stricmp(".xht",c) || !stricmp(".xhtml",c))
			{
				Book *book = new Book();
				books.push_back(book);
				book->SetFolderName(bookdir.c_str());
			
				book->SetFileName(filename);
			
				sprintf(msg,"progr: indexing book '%s'.\n",
						book->GetFileName());
				iprintf(msg);
				Log(msg);

				u8 rc = book->Index(filebuf);
				if(rc == 255) {
					sprintf(msg, "fatal: cannot index book '%s'.\n",
						book->GetFileName());
					Log(msg);
					Fatal(msg);
				}
				else if(rc == 254) {
					sprintf(msg, "fatal: cannot make book parser.\n");
					Fatal(msg);
				}
				sprintf(msg, "info : book title '%s'.\n",book->GetTitle());
				Log(msg);
				bookcount++;
			}
		}
		dirclose(dp);
		sprintf(msg,"progr: %d books indexed.\n",bookcount);
		Log(msg);
	}
	sprintf(msg,"info : currentbook = %d.\n",bookcurrent);
	Log(msg);
	swiWaitForVBlank();
	
	// Read preferences again, to bind prefs to books.
	
   	if(int parseerror = prefs->Read())
	{
		sprintf(msg,"warn : can't read prefs (%d).\n",parseerror);
		Log(msg);
	}
	else Log("info : read prefs.\n");

	// Sort bookmarks for each book.
	for(u8 i = 0; i < bookcount; i++)
	{
		books[i]->GetBookmarks()->sort();
	}

	PrefsInit();
	browser_init();

	Log("progr: browsers populated.\n");

	// If typesetter failed, shop here to show console.
	if(err) while(1) swiWaitForVBlank();
	
	// Bring up the right screen.
	videoSetModeSub(MODE_5_2D);
	vramSetBankC(VRAM_C_SUB_BG);
	int bgSub = bgInitSub(2, BgType_Bmp16, BgSize_B16_256x256, 0,0);
	bgSetCenter(bgSub,0,0);
	bgRotate(bgSub,-8192);
	bgScroll(bgSub,0,256);
	bgUpdate(bgSub);

	// Reverse orientation if needed.
	if(orientation)
	{
		RotateScreens();
		ts->SwapScreens();
	}
	
	ts->SetScreen(screenright);
	ts->ClearScreen();
	mode = APP_MODE_BROWSER;
	browser_draw();

	Log("progr: browser displayed.\n");

	if(reopen && bookcurrent > -1)
	{
		if(OpenBook())
			Log("warn : could not reopen current book.\n");
		else
		{
			Log("info : reopened current book.\n");
			mode = APP_MODE_BOOK;
		}
	}
	
	swiWaitForVBlank();
	
	// start event loop.
	
	keysSetRepeat(60,2);
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
		else if (mode == APP_MODE_PREFS_FONT 
			|| mode == APP_MODE_PREFS_FONT_BOLD
			|| mode == APP_MODE_PREFS_FONT_ITALIC)
			HandleEventInFont();

		swiWaitForVBlank();
	}

	if(p)
		XML_ParserFree(p);
	exit(0);
}

void App::CycleBrightness()
{
	brightness++;
	brightness = brightness % 4;
	BGUI::get()->setBacklightBrightness(brightness);
	prefs->Write();
}

void App::RotateScreens()
{
	screenright = (u16*)BG_BMP_RAM(0);
	screenleft = (u16*)BG_BMP_RAM_SUB(0);

	int bgMain = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0,0);
	bgSetCenter(bgMain,0,0);
	bgRotate(bgMain,8192);
	bgScroll(bgMain,0,256);
	bgUpdate(bgMain);
	//decompress(splashBitmap, BG_GFX, LZ77Vram);
	
	int bgSub = bgInitSub(2, BgType_Bmp16, BgSize_B16_256x256, 0,0);
	bgSetCenter(bgSub,0,0);
	bgRotate(bgSub,8192);
	bgScroll(bgSub,0,256);
	bgUpdate(bgSub);
}

void App::Log(const char *msg)
{
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,msg);
	fclose(logfile);
	iprintf(msg);
}

void App::Log(std::string msg)
{
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,msg.c_str());
	fclose(logfile);
	iprintf(msg.c_str());
}

void App::Fatal(const char *msg)
{
	//! Log last message and spin.
	Log(msg);
	while(1) swiWaitForVBlank();
}


/* 

dslibris - an ebook reader for the Nintendo DS.

 Copyright (C) 2007-2020 Ray Haleblian (ray@haleblian.com)

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

*/

#include "app.h"

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>   // for std::sort

#include "fat.h"
#include "nds/system.h"
#include "nds/arm9/background.h"
#include "nds/arm9/input.h"

#include "ndsx_brightness.h"
#include "types.h"
#include "main.h"
#include "parse.h"
#include "book.h"
#include "button.h"
#include "text.h"
#include "version.h"

// less-than operator to compare books by title
static bool book_title_lessthan(Book* a, Book* b) {
    return strcasecmp(a->GetTitle(),b->GetTitle()) < 0;
}

void halt()
{
	while(TRUE) swiWaitForVBlank();
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

	cache = false;
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
	char msg[512];
	SetBrightness(0);	

	Log("--------------------\n");
	Log("dslibris starting up\n");
	console = true;

	// Read preferences, pass 1,
	// to get the book folder and preferred fonts.

   	if (int err = prefs->Read())
	{
		if(err == 255)
		{
			err = prefs->Write();
			if (err) {
				Log("could not create preferences\n");
				return err;
			}
			Log("created preferences\n");
		}
	}

	// Start up typesetter.

   	int err = ts->Init();
	switch(err)
	{
		case 0:
		sprintf(msg, "typesetter started\n");
		break;
		default:
		sprintf(msg, ErrorString(err));
		// case 1:
		// sprintf(msg, "fatal: font file not found (%d)\n", err);
		// break;
		// case 2:
		// sprintf(msg, "fatal: font file unreadable (%d)\n", err);
		// break;
		// default:
		// sprintf(msg, "fatal: freetype error (%d)\n", err);
	}
	Log(msg);
	if(err) while(1) swiWaitForVBlank();
	
	SetBrightness(brightness);
	
	// Look in the book directory and construct library.

	sprintf(msg,"scanning '%s' for books\n",bookdir.c_str());
	Log(msg);
	
	DIR *dp = opendir(bookdir.c_str());
	if (!dp)
	{
		sprintf(msg,"fatal: No book directory \'%s\'.\n",
			bookdir.c_str());
		Log(msg);
	}
	
	struct dirent *ent;
	while ((ent = readdir(dp)))
	{
		char *filename = ent->d_name;
		sprintf(msg,"%s\n", filename);
		Log(msg);
		if(*filename == '.') continue;
		char *c;
		// FIXME use std::string method
		for (c=filename+strlen(filename)-1;
		     c!=filename && *c!='.';
		     c--);
		if (!strcmp(".xht",c) || !strcmp(".xhtml",c))
		{
			Book *book = new Book();
			book->SetFolderName(bookdir.c_str());
			book->SetFileName(filename);
			book->format = FORMAT_XHTML;
			books.push_back(book);
			bookcount++;
			
			Log("indexing '%s'\n", book->GetFileName());

			u8 rc = book->Index();
			if (rc)
			{
				sprintf(msg,"warn : indexer failed (%d)\n",
					rc);
			}
			else
			{
				sprintf(msg, "indexed title '%s'\n",
					book->GetTitle());
			}
			Log(msg);
		}
		else if (!strcmp(".epub",c))
		{
			Book *book = new Book();
			book->SetFolderName(bookdir.c_str());
			book->SetFileName(filename);
			book->SetTitle(filename);
			book->format = FORMAT_EPUB;
			books.push_back(book);
			bookcount++;
			book->Index();
		}
	}
	closedir(dp);
	sprintf(msg,"%d books indexed.\n",bookcount);
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

		key.downrepeat = keysDownRepeat();

		if (key.downrepeat)
		{
			switch (mode){
					case APP_MODE_BROWSER:
						HandleEventInBrowser();
						break;
					case APP_MODE_BOOK:
						HandleEventInBook();
						//UpdateClock();
						break;
					case APP_MODE_PREFS:
						HandleEventInPrefs();
						break;
					case APP_MODE_PREFS_FONT:
					case APP_MODE_PREFS_FONT_BOLD:
					case APP_MODE_PREFS_FONT_ITALIC:
						HandleEventInFont();
						break;
			}
		}
		swiWaitForVBlank();
	}

	exit(0);
}

void App::SetBrightness(int b)
{
	if(b<0) brightness = 0;
	brightness = b%4;
	fifoSendValue32(BACKLIGHT_FIFO,brightness);
}

void App::CycleBrightness()
{
	++brightness%=4;
	SetBrightness(brightness);
}

void App::UpdateClock()
{
	if (mode != APP_MODE_BOOK)
		return;
	u16 *screen = ts->GetScreen();
	time_t unixTime = time(NULL);
	struct tm* timeStruct = gmtime((const time_t *)&unixTime);

	char tmsg[8];
	ts->SetScreen(ts->screenleft);
	sprintf(tmsg, "%02d:%02d",timeStruct->tm_hour,timeStruct->tm_min );
	u8 offset = ts->margin.left;
	ts->ClearRect(offset, 240, offset+30, 255);
	ts->SetPen(offset,250);
	ts->PrintString(tmsg);

	ts->SetScreen(screen);
}

void App::SetOrientation(bool flip)
{
	s16 s;
	s16 c;
	if(flip)
	{
		s = 1 << 8;
		c = 0;
		REG_BG3X = 191 << 8;
		REG_BG3Y = 0 << 8;
		REG_BG3X_SUB = 191 << 8;
		REG_BG3Y_SUB = 0 << 8;
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
		s = -1 << 8;
		c = 0;
		REG_BG3X = 0 << 8;
		REG_BG3Y = 255 << 8;
		REG_BG3X_SUB = 0 << 8;
		REG_BG3Y_SUB = 255 << 8;
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
	REG_BG3PA = c;
	REG_BG3PB = -s;
	REG_BG3PC = s;
	REG_BG3PD = c;
	REG_BG3PA_SUB = c;
	REG_BG3PB_SUB = -s;
	REG_BG3PC_SUB = s;
	REG_BG3PD_SUB = c;
}

void App::Log(const char *msg)
{
	Log("%s",msg);
}

void App::Log(const char *format, const int value)
{
	std::stringstream ss;
	ss << value << std::endl;
	Log(format, ss.str().c_str());
}

void App::Log(const char *format, const char *msg)
{
	if(console)
	{
		char s[1024];
		sprintf(s,format,msg);
		iprintf(s);
	}
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,format,msg);
	fclose(logfile);
}

void App::Log(const std::string msg)
{
	Log("%s",msg.c_str());
}

void App::InitScreens()
{
	videoSetMode(MODE_5_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0,0);
	videoSetModeSub(MODE_5_2D);
	vramSetBankC(VRAM_C_SUB_BG);
	bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0,0);
	ts->SetScreen(ts->screenright);
	ts->ClearScreen();
	ts->SetScreen(ts->screenleft);
	ts->ClearScreen();
	SetOrientation(orientation);
}

void App::Fatal(const char *msg)
{
	Log(msg);
	while(1) swiWaitForVBlank();
}


void App::PrintStatus(const char *msg)
{
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

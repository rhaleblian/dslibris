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

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>   // for std::sort

#include <fat.h>
#include "nds.h"

#include "book.h"
#include "button.h"
#include "log.h"
#include "main.h"
#include "parse.h"
#include "text.h"
#include "types.h"
#include "version.h"

#include "app.h"

// less-than operator to compare books by title
static bool book_title_lessthan(Book* a, Book* b) {
    return strcasecmp(a->GetTitle(),b->GetTitle()) < 0;
}

void halt()
{
	while(pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();
	}
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

void DrawRect(u16 bg, u16 x0, u16 y0, u16 x1, u16 y1, u16 c) {
	u16* fb = bgGetGfxPtr(bg);
	for (u16 y=y0; y<y1; y++)
		for (u16 x=x0; x<x1; x++)
			fb[y*SCREEN_WIDTH+x] = c;
}

u8 App::Init()
{
	char msg[512];
	int err = 0;

	videoSetMode(MODE_5_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	bg[0] = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
	setBackdropColor(ARGB16(1, 29, 29, 29));
	// drawstack(bgGetGfxPtr(bg[0]));

	// videoSetModeSub(MODE_5_2D);
	// vramSetBankC(VRAM_C_SUB_BG);
	// bg[1] = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
	// setBackdropColorSub(ARGB16(1, 29, 29, 29));
	// ClearScreen(bg[1]);
	
	consoleDemoInit();

	bool fs = fatInitDefault();
	if (fs) printf("[ OK ] filesystem\n");
	else printf("[FAIL] filesystem\n");

	// Log("--------------------\n");
	// Log("dslibris starting up\n");

	// Read preferences, pass 1,
	// to get the book folder and preferred fonts.

	err = prefs->Read();
	if (err)
	{
		if(err == 255)
		{
			err = prefs->Write();
			if (err) {
				printf("[FAIL] could not create preferences\n");
			}
		}
	}
	printf("[ OK ] preferences\n");

	// Start up typesetter.

   	err = ts->Init();
	if (err)
	{
		sprintf(msg, ErrorString(err));
		printf("[FAIL] %s\n", msg);
	}
	printf("[ OK ] typesetter\n");

	u8 fontcount = ts->GetFontCount();
	if (fontcount == 0)
		printf("[FAIL] 0 fonts\n");
	else
		printf("[ OK ] %d fonts\n", fontcount);
	printf("[ OK ] well ...\n");
	u8 a = ts->GetAdvance('a');
	printf("[ OK ] a advances %d\n", a);

	// SetBrightness(brightness);
	ClearScreen(bg[0]);

	u16* screen = (u16*)bgGetGfxPtr(bg[0]);
	ts->screenleft = screen;
	ts->screenright = screen;
	ts->SetScreen(screen);
	u16 x = 10;
	u16 y = 10;
	ts->SetPen(x, y);
	err = ts->PrintChar('a');
	printf("%d\n", err);
	DrawRect(bg[0], 10, 10, 20, 20, ARGB16(1, 0, 0, 31));
	return 0;

	// Look in the book directory and construct library.

	DIR *dp = opendir(bookdir.c_str());
	if (!dp)
	{
		sprintf(msg, "no directory \'%s\'\n", bookdir.c_str());
	} else {
		sprintf(msg, "scanning '%s'\n",bookdir.c_str());
		swiWaitForVBlank();
	
		struct dirent *ent;
		while ((ent = readdir(dp)))
		{
			char *filename = ent->d_name;
			if(*filename == '.') continue;
			char *c;
			for (c=filename+strlen(filename)-1;
				 c!=filename && *c!='.';
				 c--);
			if (!strcmp(".xht", c) || !strcmp(".xhtml", c))
			{
				Book *book = new Book();
				book->SetFolderName(bookdir.c_str());
				book->SetFileName(filename);
				book->format = FORMAT_XHTML;
				books.push_back(book);
				bookcount++;
				book->Index();
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
	}
	
	sprintf(msg, "%d books\n", bookcount);
	
	// Read preferences, pass 2, to bind preferences to books.
	
   	prefs->Read();

	// Sort bookmarks for each book.
	for(u8 i = 0; i < bookcount; i++)
	{
		books[i]->GetBookmarks()->sort();
	}

	// Set up preferences editing screen.
	PrefsInit();
	
	// Set up library browser screen.
	std::sort(books.begin(),books.end(),&book_title_lessthan);
	BrowserInit();

	// Pause a moment on the console.
	u8 countdown = 60;
	while (pmMainLoop() && countdown) {
		swiWaitForVBlank();
		countdown--;
	}
	
	// if(orientation) lcdSwap();
	// if (prefs->swapshoulder)
	// {
	// 	int tmp = key.l;
	// 	key.l = key.r;
	// 	key.r = tmp;
	// }

	// BrowserDraw();
	// mode = APP_MODE_BROWSER;

	// Resume reading from the last session.
	
	// if(reopen && bookcurrent)
	// {
	// 	int openerr = OpenBook();
	// 	if(openerr)
	// 		Log("warn : could not reopen previous book.\n");
	// 	else
	// 	{
	// 		Log("info : reopened previous book.\n");
	// 		mode = APP_MODE_BOOK;
	// 	}
	// }
	// else Log("info : not reopening previous book.\n");

	// Start polling event loop.
	// FIXME use interrupt driven event handling.
	
	keysSetRepeat(60,2);

	return 0;
}

int App::Run(void)
{
	Init();
	mode = APP_MODE_NONE;
	while (pmMainLoop())
	{
		swiWaitForVBlank();
		int keys = keysDown();
		if(keys & KEY_START) break;
		scanKeys();
		key.downrepeat = keysDownRepeat();
		if (key.downrepeat)
		{
			switch (mode) {
				case APP_MODE_BROWSER:
				HandleEventInBrowser();
				break;
				case APP_MODE_BOOK:
				HandleEventInBook();
				break;
				case APP_MODE_PREFS:
				HandleEventInPrefs();
				break;
				case APP_MODE_PREFS_FONT:
				case APP_MODE_PREFS_FONT_BOLD:
				case APP_MODE_PREFS_FONT_ITALIC:
				HandleEventInFont();
				break;
				case APP_MODE_NONE:
				break;
			}
		}
	}
	return 0;
}

void App::SetBrightness(u8 b)
{
	brightness = b % 4;
	setBrightness(3, brightness);
}

void App::CycleBrightness()
{
	++brightness %= 4;
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

void App::InitScreen()
{
	videoSetMode(MODE_5_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	bg[0] = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
}

void App::InitScreenSub()
{
	videoSetModeSub(MODE_5_2D);
	vramSetBankC(VRAM_C_SUB_BG);
	bg[1] = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

}
	// ts->SetScreen(ts->screenright);
	// ts->ClearScreen();
	// ts->SetScreen(ts->screenleft);
	// ts->ClearScreen();
	// SetOrientation(orientation);

void App::ClearScreen(u16 bg) {
	u16 color = ARGB16(1, 0, 31, 0);
	u16* fb = (u16*)bgGetGfxPtr(bg);
	for (u8 iy=90; iy<110; iy++)
		for (u8 ix=120; ix<140; ix++)
			fb[iy*SCREEN_WIDTH+ix] = color;
	// swiCopy(&color, fb, 1 | COPY_MODE_HWORD | COPY_MODE_FILL);
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

	ts->ClearRect(0,top,ts->display.width,ts->display.height);
	ts->SetPen(10,top+10);
	ts->PrintString(msg);

	ts->SetPixelSize(pixelsize);
	ts->SetScreen(screen);
	ts->SetInvert(invert);
}

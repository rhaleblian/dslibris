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
#include <fat.h>

#include "main.h"
#include "parse.h"
#include "book.h"
#include "button.h"
#include "text.h"
#include "version.h"

App::App()
{
	melonds = false;
	FILE *fd = fopen("/melonds.txt", "r");
	if (fd)
	{
		fclose(fd);
		melonds = true;
	}

	fontdir = std::string("font");
	bookdir = std::string("book");
	bookcount = 0;
	bookselected = NULL;
	bookcurrent = NULL;
	reopen = true;
	mode = APP_MODE_BROWSER;
	browserstart = 0;
	cache = false;
	orientation = false;  //! turned right?
	paraspacing = 1;
	paraindent = 0;
	brightness = 1;
	invert = false;

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

	browser_view_dirty = false;
	
	font_view_dirty = false;
	font_view_initialized = false;

	prefs = new Prefs(this);
	prefsSelected = -1;
	prefs_view_dirty = false;

	browser_view_dirty = false;
	prefs_view_dirty = false;
	font_view_dirty = false;

	ts = new Text();
	ts->app = this;
}

App::~App()
{
	if (prefs) delete prefs;
	if (ts) delete ts;
	for(std::vector<Book*>::iterator it=books.begin();it!=books.end();it++)
		delete *it;
	books.clear();
}

// std::sort comparator: books by title
static bool book_title_lessthan(Book* a, Book* b)
{
    return strcasecmp(a->GetTitle(),b->GetTitle()) < 0;
}

int App::Run(void)
{
	const int ok = 0;

	// Start up typesetter.

	if (ts->Init() != ok)
		halt("[FAIL] typesetter\n");
	
	// Traverse the book directory and construct library.

	if (FindBooks() != ok)
		halt("[FAIL] no book directory\n");
	if (bookcount == 0)
		halt("[FAIL] no books\n");
	std::sort(books.begin(),books.end(),&book_title_lessthan);

	// Read and apply preferences.

	prefs->Read();
	prefs->Apply();

	// Sort bookmarks for each book.
	for(u8 i = 0; i < bookcount; i++)
	{
		books[i]->GetBookmarks()->sort();
	}

	// Set up settings screen.
	PrefsInit();
	
	// Set up library screen.
	browser_init();

	// Bring up displays.

	InitScreens();
	ts->PrintSplash(ts->screenleft);
	mode = APP_MODE_BROWSER;
	browser_draw();

	// Resume reading from the last session.
	
	if(reopen && bookcurrent) if (OpenBook()) browser_draw();

	keysSetRepeat(60,2);
	while (pmMainLoop())
	{
		swiWaitForVBlank();
		scanKeys();

		switch (mode) {
			case APP_MODE_BOOK:
			HandleEventInBook();
			break;

			case APP_MODE_BROWSER:
			browser_handleevent();
			if (browser_view_dirty) browser_draw();
			break;

			case APP_MODE_QUIT:
			prefs->Write();
			return 0;
			break;

			case APP_MODE_PREFS:
			PrefsHandleEvent();
			if (prefs_view_dirty) PrefsDraw();
			break;

			case APP_MODE_PREFS_FONT:
			case APP_MODE_PREFS_FONT_BOLD:
			case APP_MODE_PREFS_FONT_ITALIC:
			case APP_MODE_PREFS_FONT_BOLDITALIC:
			FontHandleEvent();
			if (font_view_dirty) FontDraw();
			break;
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

int App::FindBooks() {
	DIR *dp = opendir(bookdir.c_str());
	if (!dp)
		return 1;
	struct dirent *ent;
	while ((ent = readdir(dp)))
	{
		char *filename = ent->d_name;
		if(*filename == '.') continue;
		// Starting from the end, find the file extension.
		char *c;
		for (c=filename+strlen(filename)-1;
		     c!=filename && *c!='.';
		     c--);
		if (!strcmp(".epub",c))
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
	return 0;
}

//! Just like libnds touchRead() but takes orientation into account.
touchPosition App::TouchRead() {
	touchPosition touch;
	touchRead(&touch);
	touchPosition coord;

	if(!orientation)
	{
		coord.px = 256 - touch.px;
		coord.py = touch.py;
	} else {
		coord.px = touch.px;
		coord.py = 192 - touch.py;
	}
	return coord;
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

void App::SetOrientation(bool flipped)
{
	s16 s;
	s16 c;
	if(flipped)
	{
		s = 1 << 8;
		c = 0;
		REG_BG3X = 191 << 8;
		REG_BG3Y = 0 << 8;
		REG_BG3X_SUB = 191 << 8;
		REG_BG3Y_SUB = 0 << 8;
		ts->screenright = (u16*)BG_BMP_RAM_SUB(0);
		ts->screenleft = (u16*)BG_BMP_RAM(0);
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
	if(invert) {
		lcdSwap();
	}
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

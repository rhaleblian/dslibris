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

#include <algorithm>   // for std::sort
#include <errno.h>
#include <fat.h>
#include <nds.h>
// #include "nds/arm9/background.h"
// #include "nds/arm9/input.h"
// #include "nds/system.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <unistd.h>
#include "book.h"
#include "browser.h"
#include "page.h"
#include "parse.h"
#include "prefs.h"
// #include "splash.h"
#include "text.h"
#include "version.h"

#include "app.h"

void halt() {
	//! Flush video and loop indefinitely.
	swiWaitForVBlank();
	while(pmMainLoop()) scanKeys(); 
}

App::App()
{	
	prefs = new Prefs(this);
	ts = new Text(this);
	browser = nullptr;

	fontdir = std::string(FONTDIR);
	bookdir = std::string(BOOKDIR);
	bookcurrent = NULL;
	bookmaxbuttons = BOOK_BUTTON_COUNT;
	reopen = true;
	mode = APP_MODE_BROWSER;
	cache = false;
	brightness = 2;
	bg[0] = bg[1] = 0;
	SetControls();
}

App::~App()
{
	delete prefs;
	delete ts;
	std::vector<Book*>::iterator it;
	for(it=books.begin();it!=books.end();it++)
		delete *it;
	books.clear();
}

u8 App::Init()
{
	videoSetMode(MODE_5_2D);
	videoBgEnable(3);
	vramSetBankA(VRAM_A_MAIN_BG);
	setBackdropColor(ARGB16(1,15,15,15));
	bg[0] = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

	videoSetModeSub(MODE_5_2D);
	videoBgEnableSub(2);
	vramSetBankC(VRAM_C_SUB_BG);
	setBackdropColorSub(ARGB16(1,15,15,15));
	bg[1] = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

	u8 error = fatInitDefault();

	ts->screenleft = bgGetGfxPtr(bg[0]);
	ts->screenright = bgGetGfxPtr(bg[1]);
	ts->screen = ts->screenleft;
	ts->Init();
	printf("typesetter\n");
	ts->ReportFace(TEXT_STYLE_REGULAR);

	// DrawSplashScreen();

	// Read preferences to get the book folder and preferred fonts.
	error = prefs->Read();
	if(error == PREFS_ERROR_MISSING)
		error = prefs->Write();
	if (error) return error;

	// SetOrientation(orientation);
	// if(orientation) lcdSwap();

	// Traverse the book directory and construct library.
	IndexBooks();

	// Read preferences again to apply to books.
	error = prefs->Read();
	if (error) return error;

	// SortBooks();
	// SortBookmarks();

	// Set up preferences editing screen.
	// PrefsInit();

	// Set up the library browsing screen.
	browser = new Browser(this);

	u16 *screen = bgGetGfxPtr(bg[1]);
	for (int iy=-16;iy<16;iy++)
		for (int ix=-16;ix<16;ix++)
			screen[(SCREEN_HEIGHT/2+iy)*256 + ix+SCREEN_WIDTH/2] = ARGB16(1, 30, 30, 30);
	swiWaitForVBlank();

	FILE* fp = fopen("/dslibris.log", "w");
	auto face = ts->faces[TEXT_STYLE_BROWSER];
	fprintf(fp, "name: %s\n", face->family_name);
	fprintf(fp, "charmaps: %d\n", face->num_charmaps);
	fclose(fp);

	return 0;

	browser->Init();
	browser->Draw();

	// if (prefs->swapshoulder)
	// {
	// 	int tmp = key.l;
	// 	key.l = key.r;
	// 	key.r = tmp;
	// }
	// keysSetRepeat(60,2);
	// mode = APP_MODE_BROWSER;

	return error;
}

u8 App::Init2() {
	consoleDemoInit();
	printf("console\n");
	for (auto name : ts->filenames)
		printf("%d %s\n", name.first, name.second.data());
	if (!fatInitDefault()) {
		printf("no filesytem\n");
		return 255;
	}
	printf("filesystem\n");

	u8 error = ts->Init();
	if (error)
		printf("no typesetter\n");
	else
		printf("typesetter\n");
	swiWaitForVBlank();
	ts->ReportFace(TEXT_STYLE_REGULAR);
	swiWaitForVBlank();
	
	// ts->SetStyle(TEXT_STYLE_BROWSER);
	// printf("set browser font\n");
	// swiWaitForVBlank();
	
	return 0;
}

int App::Run(void)
{
	Init2();
	while (pmMainLoop())
	{
		threadWaitForVBlank();
		scanKeys();
		// key.downrepeat = keysDownRepeat();
		// if (key.downrepeat)
		// {
		// 	switch (mode){
		// 			case APP_MODE_BROWSER:
		// 				HandleEventInBrowser();
		// 				break;
		// 			case APP_MODE_BOOK:
		// 				HandleEventInBook();
		// 				break;
		// 			case APP_MODE_PREFS:
		// 				HandleEventInPrefs();
		// 				break;
		// 			case APP_MODE_PREFS_FONT:
		// 			case APP_MODE_PREFS_FONT_BOLD:
		// 			case APP_MODE_PREFS_FONT_ITALIC:
		// 				HandleEventInFont();
		// 				break;
		// 			default:
		// 				break;
		// 	}
		// }
	}
	return 0;
}


void App::ClearScreen(u16 *screen, u8 r, u8 g, u8 b)
{
	//! Must be a 16-bit background.
	u16 src = ARGB16(1, 15, 15, 15);
	u16* dst = bgGetGfxPtr(bg[1]);
	// swiCopy(&color, screen), 1 | COPY_MODE_HWORD | COPY_MODE_FILL);
	for (int i=0; i<SCREEN_WIDTH*SCREEN_WIDTH; i++) dst[i] = src;
}

void App::DrawSplashScreen() {
	// Copy the splash screen to the background.
	// dmaCopy(splashBitmap, bgGetGfxPtr(bg[0]), splashBitmapLen);
	// dmaCopy(splashPal, BG_PALETTE, splashPalLen);
	// bgSetCenter(bg[0], 256, 0);
	// bgSetRotate(bg[0], -32767/4);
	// // bgSetScroll(bg[0], 31, 31);
	// bgUpdate();
}

void App::IndexBooks() {
	DIR *dp = opendir(bookdir.c_str());
	if (!dp) return;

	struct dirent *ent;
	while ((ent = readdir(dp)))
	{
		char *filename = ent->d_name;
		printf("%s\n", filename);
		if(*filename == '.') continue;
		char *c;
		// FIXME use std::string method
		for (c=filename+strlen(filename)-1;
			c!=filename && *c!='.';
			c--);
		if (!strcmp(".xht",c) || !strcmp(".xhtml",c))
		{
			Book *book = new Book(this);
			book->SetFolderName(bookdir.c_str());
			book->SetFileName(filename);
			book->format = FORMAT_XHTML;
			books.push_back(book);
			book->Index();
		}
		else if (!strcmp(".epub",c))
		{
			Book *book = new Book(this);
			book->SetFolderName(bookdir.c_str());
			book->SetFileName(filename);
			book->SetTitle(filename);
			book->format = FORMAT_EPUB;
			books.push_back(book);
			book->Index();
		}
	}
	closedir(dp);
}

void App::SetBrightness(int b)
{
	brightness = b % 4;
	setBrightness(3, brightness);
}

void App::CycleBrightness()
{
	++brightness %= 4;
	SetBrightness(brightness);
}

void App::SetOrientation(bool flipped)
{
	// s16 s;
	// s16 c;
	ts->orientation = flipped;
	if(ts->orientation)
	{
		// s = 1 << 8;
		// c = 0;
		// REG_BG3X = 191 << 8;
		// REG_BG3Y = 0 << 8;
		// REG_BG3X_SUB = 191 << 8;
		// REG_BG3Y_SUB = 0 << 8;
		ts->screenleft = (u16*)bgGetGfxPtr(bg[0]);
		ts->screenright = (u16*)bgGetGfxPtr(bg[1]);
	}
	else
	{
		// s = -1 << 8;
		// c = 0;
		// REG_BG3X = 0 << 8;
		// REG_BG3Y = 255 << 8;
		// REG_BG3X_SUB = 0 << 8;
		// REG_BG3Y_SUB = 255 << 8;
		ts->screenleft = (u16*)bgGetGfxPtr(bg[1]);
		ts->screenright = (u16*)bgGetGfxPtr(bg[0]);
	}
	// REG_BG3PA = c;
	// REG_BG3PB = -s;
	// REG_BG3PC = s;
	// REG_BG3PD = c;
	// REG_BG3PA_SUB = c;
	// REG_BG3PB_SUB = -s;
	// REG_BG3PC_SUB = s;
	// REG_BG3PD_SUB = c;
	SetControls(ts->orientation);
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

void App::ReopenBook() {
	// Resume reading from the last session.
	if(reopen && bookcurrent)
	{
		int openerr = OpenBook(bookcurrent);
		if(!openerr)
			mode = APP_MODE_BOOK;
	}
}

static bool book_title_lessthan(Book* a, Book* b) {
	// less-than operator to compare books by title
	return strcasecmp(a->GetTitle(),b->GetTitle()) < 0;
}

void App::SortBooks() {
	std::sort(books.begin(), books.end(), &book_title_lessthan);
}

void App::SortBookmarks() {
	for(u8 i = 0; i < books.size(); i++)
	{
		books[i]->GetBookmarks()->sort();
	}
}

// void App::Fatal(const char *msg)
// {
// 	Log(msg);
// 	swiWaitForVBlank();
//     while(pmMainLoop()) scanKeys();
// }

u8 App::GetBookIndex(Book *b)
{
	if (!b) return -1;
	int i = 0;
	for(std::vector<Book*>::iterator it=books.begin();
		it<books.end();it++,i++)
	{
		if(*it == b) return i;
	}
	return -1;
}

u8 App::OpenBook(Book *book)
{
	//! Attempt to open book indicated by bookselected.
	if (bookcurrent == book)
	{
		PrintStatus("book already open");
		return 0;
	}
	if (!book)
	{
		PrintStatus("no book selected");
		return 255;
	}
	if (!book->GetFileName())
	{
		PrintStatus("no book filename");
		return 255;
	}
	if (!book->GetFolderName())
	{
		PrintStatus("no book folder");
		return 255;
	}
	if (bookcurrent) {
		bookcurrent->Close();
		bookcurrent = nullptr;
	}

	PrintStatus("opening book...");
	swiWaitForVBlank();

	const char *filename = book->GetFileName();
	const char *c; 	// will point to the file's extension.
	for (c=filename;c!=filename+strlen(filename) && *c!='.';c++);
	
	if(bookcurrent) bookcurrent->Close();
	if (int err = book->Open())
	{
		char msg[64];
		sprintf(msg, "could not open book (%d)",err);
		PrintStatus(msg);
		return 255;
	}
	PrintStatus("book opened");
	bookcurrent = book;
	if(mode == APP_MODE_BROWSER) {
		if(ts->orientation) lcdSwap();
		mode = APP_MODE_BOOK;
	}
	if(bookcurrent->GetPosition() >= bookcurrent->GetPageCount())
		bookcurrent->SetPosition(0);
	bookcurrent->GetPage()->Draw(ts);
	prefs->Write();
	ts->PrintStats();
	return 0;
}

void App::SetControls(bool flipped) {
	key.a = KEY_A;
	key.b = KEY_B;
	key.x = KEY_X;
	key.y = KEY_Y;
	key.start = KEY_START;
	key.select = KEY_SELECT;
	if (flipped) {
		key.down = KEY_UP;
		key.up = KEY_DOWN;
		key.left = KEY_RIGHT;
		key.right = KEY_LEFT;
		key.l = KEY_R;
		key.r = KEY_L;
	} else {
		key.down = KEY_DOWN;
		key.up = KEY_UP;
		key.left = KEY_LEFT;
		key.right = KEY_RIGHT;
		key.start = KEY_START;
		key.select = KEY_SELECT;
		key.l = KEY_L;
		key.r = KEY_R;
	}
}

void App::FontInit()
{
	DIR *dp = opendir(fontdir.c_str());
	if (!dp)
	{
		// Log("fatal: no font directory.\n");
		swiWaitForVBlank();
		exit(-3);
	}
	
	ts->fontPage = 0;
	ts->fontSelected = 0;
	ts->fontButtons.clear();
	
	struct dirent *ent;
	while ((ent = readdir(dp)))
	{	
		// Don't try folders
		if (ent->d_type == DT_DIR)
			continue;
		char *filename = ent->d_name;
		char *c;
		for (c=filename; c != filename + strlen(filename) && *c != '.'; c++);
		if (!strcmp(".ttf",c) || !strcmp(".otf",c) || !strcmp(".ttc",c))
		{
			Button *b = new Button();
			b->Init(ts);
			b->Move(2, (ts->fontButtons.size() % bookmaxbuttons) * 32);
			b->Label(filename);
 			ts->fontButtons.push_back(b);
		}
	}
	closedir(dp);
}

void App::HandleEventInFont()
{
	if (keysDown() & (KEY_START | KEY_SELECT | KEY_B)) {
		mode = APP_MODE_PREFS;
		FontDeinit();
		PrefsDraw();
	} else if (ts->fontSelected > 0 && (keysDown() & (KEY_RIGHT | KEY_R))) {
		if (ts->fontSelected == ts->fontPage * 7) {
			FontPreviousPage();
			FontDraw();
		} else {
			ts->fontSelected--;
			FontDraw(false);
		}
	} else if (ts->fontSelected < (ts->fontButtons.size() - 1) && (keysDown() & (KEY_LEFT | KEY_L))) {
		if (ts->fontSelected == ts->fontPage * bookmaxbuttons + (bookmaxbuttons-1)) {
			FontNextPage();
			FontDraw();
		} else {
			ts->fontSelected++;
			FontDraw(false);
		}
	} else if (keysDown() & KEY_A) {
		FontButton();
	} else if (keysDown() & KEY_TOUCH) {
		// Log("info : font screen touched\n");
		touchPosition touch;
		touchRead(&touch);
		touchPosition coord;

		if(!ts->orientation)
		{
			coord.px = 256 - touch.px;
			coord.py = touch.py;
		} else {
			coord.px = touch.px;
			coord.py = 192 - touch.py;
		}

		if(buttonnext.EnclosesPoint(coord.py,coord.px)){
			FontNextPage();
			FontDraw();
		} else if(buttonprev.EnclosesPoint(coord.py,coord.px)) {
			FontPreviousPage();
			FontDraw();
		} else if(buttonprefs.EnclosesPoint(coord.py,coord.px)) {
			mode = APP_MODE_PREFS;
			FontDeinit();
			PrefsDraw();
		} else {
			for(u8 i = ts->fontPage * 7; (i < ts->fontButtons.size()) && (i < (ts->fontPage + 1) * bookmaxbuttons); i++) {
				// Log("info : checking button\n");
				if (prefsButtons[i]->EnclosesPoint(coord.py, coord.px))
				{
					if (i != ts->fontSelected) {
						ts->fontSelected = i;
						FontDraw(false);
					}
					
					FontButton();
					break;
				}
			}
		}
	}
}

void App::FontDeinit()
{
	for (u8 i = 0; i < ts->fontButtons.size(); i++) {
		if(ts->fontButtons[i]) delete ts->fontButtons[i];
	}
	ts->fontButtons.clear();
}

void App::FontDraw(bool redraw)
{
	// save state.
	bool invert = ts->GetInvert();
	u16* screen = ts->GetScreen();
	int style = ts->GetStyle();
	
	ts->SetInvert(false);
	ts->SetStyle(TEXT_STYLE_BROWSER);
	if (redraw) {
		ts->ClearScreen();
	}
	u8 s = ts->fontButtons.size();
	for (u8 i = ts->fontPage * 7; (i < s) && (i < (ts->fontPage + 1) * bookmaxbuttons); i++)
	{
		ts->fontButtons[i]->Draw(ts->screenright, i == ts->fontSelected);
	}
	// buttonprefs.Label("Cancel");
	// buttonprefs.Draw(screen, false);
	// if(s > (ts->fontPage + 1) * bookmaxbuttons)
	// 	buttonnext.Draw(ts->screenright, false);
	// if(ts->fontSelected > bookmaxbuttons)
	// 	buttonprev.Draw(ts->screenright, false);

	// restore state.
	ts->SetStyle(style);
	ts->SetInvert(invert);
	ts->SetScreen(screen);
}

void App::FontNextPage()
{
	u8 s = ts->fontButtons.size();
	if((ts->fontPage + 1) * bookmaxbuttons < s)
	{
		ts->fontPage += 1;
		ts->fontSelected = ts->fontPage * bookmaxbuttons;
	}
}

void App::FontPreviousPage()
{
	if(ts->fontPage > 0)
	{
		ts->fontPage--;
		ts->fontSelected = ts->fontPage * bookmaxbuttons + (bookmaxbuttons-1);
	}
}

void App::FontButton()
{
	bool invert = ts->GetInvert();

	ts->SetScreen(ts->screenright);
	ts->SetInvert(false);
	ts->ClearScreen();
	ts->SetPen(ts->margin.left,PAGE_HEIGHT/2);
	ts->PrintString("[saving font...]");
	ts->SetInvert(invert);

	std::string path = fontdir;
	path.append(ts->fontButtons[ts->fontSelected]->GetLabel());
	if (mode == APP_MODE_PREFS_FONT)
		ts->SetFontFile(path.c_str(), TEXT_STYLE_REGULAR);
	else if (mode == APP_MODE_PREFS_FONT_BOLD)
		ts->SetFontFile(path.c_str(), TEXT_STYLE_BOLD);
	else if (mode == APP_MODE_PREFS_FONT_ITALIC)
		ts->SetFontFile(path.c_str(), TEXT_STYLE_ITALIC);

	ts->Init();
	bookcurrent = NULL; //Force repagination
	FontDeinit();
	mode = APP_MODE_PREFS;
	PrefsRefreshButtonFont();
	PrefsRefreshButtonFontBold();
	PrefsRefreshButtonFontItalic();
	PrefsDraw();
	prefs->Write();

	ts->SetInvert(invert);
}

void App::PrefsInit()
{	
	prefsButtonFont.Init(ts);
	prefsButtonFont.Move(2, PREFS_BUTTON_FONT * 32);
	PrefsRefreshButtonFont();
	prefsButtons[PREFS_BUTTON_FONT] = &prefsButtonFont;
	
	prefsButtonFontBold.Init(ts);
	prefsButtonFontBold.Move(2, PREFS_BUTTON_FONT_BOLD * 32);
	PrefsRefreshButtonFontBold();
	prefsButtons[PREFS_BUTTON_FONT_BOLD] = &prefsButtonFontBold;
		
	prefsButtonFontItalic.Init(ts);
	prefsButtonFontItalic.Move(2, PREFS_BUTTON_FONT_ITALIC * 32);
	PrefsRefreshButtonFontItalic();
	prefsButtons[PREFS_BUTTON_FONT_ITALIC] = &prefsButtonFontItalic;
	
	prefsButtonFontSize.Init(ts);
	prefsButtonFontSize.Move(2, PREFS_BUTTON_FONTSIZE * 32);
	PrefsRefreshButtonFontSize();
	prefsButtons[PREFS_BUTTON_FONTSIZE] = &prefsButtonFontSize;
	
	prefsButtonParaspacing.Init(ts);
	prefsButtonParaspacing.Move(2, PREFS_BUTTON_PARASPACING * 32);
	PrefsRefreshButtonParaspacing();
	prefsButtons[PREFS_BUTTON_PARASPACING] = &prefsButtonParaspacing;
	
	prefsSelected = 0;
}

void App::PrefsRefreshButtonFont()
{
	char msg[128];
	strcpy(msg, "");
	sprintf((char*)msg, "Regular Font:\n %s", ts->GetFontFile(TEXT_STYLE_REGULAR).c_str());
	prefsButtonFont.Label(msg);
}

void App::PrefsRefreshButtonFontBold()
{
	char msg[128];
	strcpy(msg, "");
	sprintf((char*)msg, "Bold Font:\n %s", ts->GetFontFile(TEXT_STYLE_BOLD).c_str());
	prefsButtonFontBold.Label(msg);
}

void App::PrefsRefreshButtonFontItalic()
{
	char msg[128];
	strcpy(msg, "");
	sprintf((char*)msg, "Italic Font:\n %s", ts->GetFontFile(TEXT_STYLE_ITALIC).c_str());
	prefsButtonFontItalic.Label(msg);
}

void App::PrefsRefreshButtonFontSize()
{
	char msg[30];
	strcpy(msg, "");
	if (ts->GetPixelSize() == 1)
		sprintf((char*)msg, "Font Size:\n    [ %d >", ts->GetPixelSize());
	else if (ts->GetPixelSize() == 255)
		sprintf((char*)msg, "Font Size:\n    < %d ]", ts->GetPixelSize());
	else
		sprintf((char*)msg, "Font Size:\n    < %d >", ts->GetPixelSize());
	prefsButtonFontSize.Label(msg);
}

void App::PrefsRefreshButtonParaspacing()
{
	char msg[38];
	strcpy(msg, "");
	if (ts->paraspacing == 0)
		sprintf((char*)msg, "Paragraph Spacing:\n    [ %d >", ts->paraspacing);
	else if (ts->paraspacing == 255)
		sprintf((char*)msg, "Paragraph Spacing:\n    < %d ]", ts->paraspacing);
	else
		sprintf((char*)msg, "Paragraph Spacing:\n    < %d >", ts->paraspacing);
	prefsButtonParaspacing.Label(msg);
}

void App::PrefsDraw()
{
	PrefsDraw(true);
}

void App::PrefsDraw(bool redraw)
{
	// save state.
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();
	u16* screen = ts->GetScreen();
	int style = ts->GetStyle();
	
	ts->SetScreen(ts->screenright);
	ts->SetInvert(false);
	ts->SetStyle(TEXT_STYLE_BROWSER);
	if (redraw) ts->ClearScreen();
	ts->SetPixelSize(PIXELSIZE);
	for (u8 i = 0; i < PREFS_BUTTON_COUNT; i++)
	{
		prefsButtons[i]->Draw(ts->screenright, i == prefsSelected);
	}
	
	buttonprefs.Label("return");
	buttonprefs.Draw(ts->screenright, false);

	// restore state.
	ts->SetStyle(style);
	ts->SetInvert(invert);
	ts->SetPixelSize(size);
	ts->SetScreen(screen);
}

void App::HandleEventInPrefs()
{
	int keys = keysDown();
	
	if (keysDown() & (KEY_START | KEY_SELECT | KEY_B)) {
		mode = APP_MODE_BROWSER;
		browser->Draw();
	} else if (prefsSelected > 0 && (keysDown() & (key.right | key.r))) {
		prefsSelected--;
		PrefsDraw(false);
	} else if (prefsSelected < PREFS_BUTTON_COUNT - 1 && (keysDown() & (KEY_LEFT | KEY_L))) {
		prefsSelected++;
		PrefsDraw(false);
	} else if (keysDown() & KEY_A) {
		PrefsButton();
	} else if (keys & KEY_Y) {
		CycleBrightness();
		prefs->Write();
	} else if (keysDown() & KEY_TOUCH) {
		touchPosition touch;
		touchRead(&touch);
		touchPosition coord;

		if(!ts->orientation)
		{
			coord.px = 256 - touch.px;
			coord.py = 192 - touch.py;
		} else {
			coord.px = touch.px;
			coord.py = touch.py;
		}
		
		if (buttonprefs.EnclosesPoint(coord.py, coord.px)) {
			buttonprefs.Label("prefs");
			mode = APP_MODE_BROWSER;
			browser->Draw();
		} else {
			for(u8 i = 0; i < PREFS_BUTTON_COUNT; i++) {
				if (prefsButtons[i]->EnclosesPoint(coord.py, coord.px))
				{
					if (i != prefsSelected) {
						prefsSelected = i;
						PrefsDraw(false);
					}
					
					if (i == PREFS_BUTTON_FONTSIZE) {
						if (coord.py < 2 + 188 / 2) {
							PrefsIncreasePixelSize();
						} else {
							PrefsDecreasePixelSize();
						}
					} else if (i == PREFS_BUTTON_PARASPACING) {
						if (coord.py < 2 + 188 / 2) {
							PrefsIncreaseParaspacing();
						} else {
							PrefsDecreaseParaspacing();
						}
					} else {
						PrefsButton();
					}
					
					break;
				}
			}
		}
	} else if (prefsSelected == PREFS_BUTTON_FONTSIZE && (keysDown() & key.up)) {
		PrefsDecreasePixelSize();
	} else if (prefsSelected == PREFS_BUTTON_FONTSIZE && (keysDown() & key.down)) {
		PrefsIncreasePixelSize();
	} else if (prefsSelected == PREFS_BUTTON_PARASPACING && (keysDown() & key.up)) {
		PrefsDecreaseParaspacing();
	} else if (prefsSelected == PREFS_BUTTON_PARASPACING && (keysDown() & key.down)) {
		PrefsIncreaseParaspacing();
	}
}

void App::PrefsIncreasePixelSize()
{
	if (ts->pixelsize < 255) {
		ts->pixelsize++;
		PrefsRefreshButtonFontSize();
		PrefsDraw();
		bookcurrent = NULL;
		prefs->Write();
	}
}

void App::PrefsDecreasePixelSize()
{
	if (ts->pixelsize > 1) {
		ts->pixelsize--;
		PrefsRefreshButtonFontSize();
		PrefsDraw();
		bookcurrent = NULL;
		prefs->Write();
	}
}

void App::PrefsIncreaseParaspacing()
{
	if (ts->paraspacing < 255) {
		ts->paraspacing++;
		PrefsRefreshButtonParaspacing();
		PrefsDraw();
		bookcurrent = NULL;
		prefs->Write();
	}
}

void App::PrefsDecreaseParaspacing()
{
	if (ts->paraspacing > 0) {
		ts->paraspacing--;
		PrefsRefreshButtonParaspacing();
		PrefsDraw();
		bookcurrent = NULL;
		prefs->Write();
	}
}

void App::PrefsButton()
{
	if (prefsSelected == PREFS_BUTTON_FONT) {
		mode = APP_MODE_PREFS_FONT;
	} else if (prefsSelected == PREFS_BUTTON_FONT_BOLD) {
		mode = APP_MODE_PREFS_FONT_BOLD;
	} else if (prefsSelected == PREFS_BUTTON_FONT_ITALIC) {
		mode = APP_MODE_PREFS_FONT_ITALIC;
	}
	PrintStatus("[loading fonts...]");
	ts->SetScreen(ts->screenright);
	ts->ClearScreen();
	FontInit();
	FontDraw();
	PrintStatus("");
}

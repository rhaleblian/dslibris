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

void App::FontInit()
{
	DIR_ITER *dp = diropen(fontdir.c_str());
	if (!dp)
	{
		Log("fatal: no font directory.\n");
		swiWaitForVBlank();
		exit(-3);
	}
	
	fontPage = 0;
	fontSelected = 0;
	
	char filename[MAXPATHLEN];
	while (fontTs.size() < MAXBOOKS)
	{
		struct stat st;
		int rc = dirnext(dp, filename, &st);
		if(rc)
			break;
		
		// Don't try folders
		if (st.st_mode & S_IFDIR)
			continue;

		char *c;
		for (c=filename; c != filename + strlen(filename) && *c != '.'; c++);
		if (!stricmp(".ttf",c))
		{
			Text* newTs = new Text();
			newTs->app = this;
			
			char newFilename[MAXPATHLEN];
			strcpy(newFilename, fontdir.c_str());
			strcat(newFilename, filename);
			newTs->SetFontFile(newFilename, 0);
			
			const char* fontfile = NULL;
			
			if (mode == APP_MODE_PREFS_FONT)
				fontfile = ts->GetFontFile().c_str();
			else if (mode == APP_MODE_PREFS_FONT_BOLD)
				fontfile = ts->GetFontBoldFile().c_str();
			else if (mode == APP_MODE_PREFS_FONT_ITALIC)
				fontfile = ts->GetFontItalicFile().c_str();
			
			if (!stricmp(newFilename, fontfile)) {
				fontSelected = fontTs.size();
				fontPage = fontSelected / 7;
			}
				
			if (newTs->Init()) {
				delete newTs;
			} else {
				newTs->SetPixelSize(12);
				fontTs.push_back(newTs);
			}
		}
	}
	dirclose(dp);

	fontButtons = new Button[fontTs.size()];
	
	for (u8 i = 0; i < fontTs.size(); i++) {
		Text* ts = fontTs[i];
		fontButtons[i].Init(ts);
		fontButtons[i].Move(2, ((i % 7) + 1) * 32 - 16);
		fontButtons[i].Label(ts->GetFontFile().c_str());
	}
}

void App::HandleEventInFont()
{
	if (keysDown() & (KEY_START | KEY_SELECT | KEY_B)) {
		mode = APP_MODE_PREFS;
		FontDeinit();
		PrefsDraw();
	} else if (fontSelected > 0 && (keysDown() & (KEY_RIGHT | KEY_R))) {
		if (fontSelected == fontPage * 7) {
			FontPreviousPage();
			FontDraw();
		} else {
			fontSelected--;
			FontDraw(false);
		}
	} else if (fontSelected < fontTs.size() - 1 && (keysDown() & (KEY_LEFT | KEY_L))) {
		if (fontSelected == fontPage * 7 + 6) {
			FontNextPage();
			FontDraw();
		} else {
			fontSelected++;
			FontDraw(false);
		}
	} else if (keysDown() & KEY_A) {
		FontButton();
	} else if (keysDown() & KEY_TOUCH) {
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
		if(coord.px > regionnext[0] && coord.px < regionnext[1])
		{
			FontNextPage();
			FontDraw();
		}
		else if(coord.px > regionprev[0] && coord.px < regionprev[1])
		{
			if (fontPage > 0) {
				FontPreviousPage();
				FontDraw();
			} else {
				mode = APP_MODE_PREFS;
				FontDeinit();
				PrefsDraw();
			}
		} else {
			for(u8 i = fontPage * 7; (i < fontTs.size()) && (i < (fontPage + 1) * 7); i++) {
				if (prefsButtons[i]->EnclosesPoint(coord.py, coord.px))
				{
					if (i != fontSelected) {
						fontSelected = i;
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
	delete[] fontButtons;
	
	for (u8 i = 0; i < fontTs.size(); i++) {
		delete fontTs[i];
	}
	
	fontTs.clear();
}

void App::FontDraw()
{
	FontDraw(true);
}

void App::FontDraw(bool redraw)
{
	// save state.
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();

#ifndef GRIT
	u16* screen;
	if (orientation)
		screen = screen0;
	else
		screen = screen1;
 
	if (redraw) {
		ts->ClearScreen(screen,0,0,0);
	}
	ts->SetPixelSize(12);
	for (u8 i = fontPage * 7; (i < fontTs.size()) && (i < (fontPage + 1) * 7); i++)
	{
		fontButtons[i].Draw(screen, i == fontSelected);
	}
	if(fontPage > 0) 
		buttonprev.Label("^");
	else
		buttonprev.Label("Cancel [Start/Select]");
	buttonprev.Draw(screen, false);
	
	if((u8)fontTs.size() > (fontPage + 1) * 7)
		buttonnext.Draw(screen, false);
#endif

	// restore state.
	ts->SetInvert(invert);
	ts->SetPixelSize(size);
}

void App::FontNextPage()
{
	if((fontPage + 1) * 7 < (u8)fontTs.size())
	{
		fontPage += 1;
		fontSelected = fontPage * 7;
	}
}

void App::FontPreviousPage()
{
	if(fontPage > 0)
	{
		fontPage--;
		fontSelected = fontPage * 7 + 6;
	}
}

void App::FontButton()
{
	ts->SetScreen(screen1);
	ts->ClearScreen(screen1,0,0,0);
	ts->SetPen(marginleft,PAGE_HEIGHT/2);
	ts->PrintString("[saving font...]");
	
	if (mode == APP_MODE_PREFS_FONT)
		ts->SetFontFile(fontTs[fontSelected]->GetFontFile().c_str(), 0);
	else if (mode == APP_MODE_PREFS_FONT_BOLD)
		ts->SetFontBoldFile(fontTs[fontSelected]->GetFontFile().c_str(), 0);
	else if (mode == APP_MODE_PREFS_FONT_ITALIC)
		ts->SetFontItalicFile(fontTs[fontSelected]->GetFontFile().c_str(), 0);
	
	ts->Init();
	bookcurrent = -1; //Force repagination
	FontDeinit();
	mode = APP_MODE_PREFS;
	PrefsRefreshButtonFont();
	PrefsDraw();
	prefs->Write();
}

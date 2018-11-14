#include <errno.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>

#include <expat.h>

#include <fat.h>
#include <nds/bios.h>

#include <string>
#include <vector>

#include "types.h"
#include "main.h"
#include "parse.h"
#include "App.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"

#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)
#define BPP 7  // buttons per page
#define BUTTONHEIGHT 32

void App::FontInit()
{
	DIR *dp = opendir(fontdir.c_str());
	if (!dp)
	{
		Log("fatal: no font directory.\n");
		swiWaitForVBlank();
		exit(-3);
	}
	
	fontPage = 0;
	fontSelected = 0;
	fontButtons.clear();
	
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
			b->Move(2, (fontButtons.size() % BPP) * 32);
			b->Label(filename);
 			fontButtons.push_back(b);
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
	} else if (fontSelected > 0 && (keysDown() & (KEY_RIGHT | KEY_R))) {
		if (fontSelected == fontPage * 7) {
			FontPreviousPage();
			FontDraw();
		} else {
			fontSelected--;
			FontDraw(false);
		}
	} else if (fontSelected < (fontButtons.size() - 1) && (keysDown() & (KEY_LEFT | KEY_L))) {
		if (fontSelected == fontPage * BPP + (BPP-1)) {
			FontNextPage();
			FontDraw();
		} else {
			fontSelected++;
			FontDraw(false);
		}
	} else if (keysDown() & KEY_A) {
		FontButton();
	} else if (keysDown() & KEY_TOUCH) {
		Log("info : font screen touched\n");
		touchPosition touch;
		touchRead(&touch);
		touchPosition coord;
		u8 regionprev[2], regionnext[2];
		regionprev[0] = 0;
		regionprev[1] = 16;
		regionnext[0] = 240;
		regionnext[1] = 255;

		if(!orientation)
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
			for(u8 i = fontPage * 7; (i < fontButtons.size()) && (i < (fontPage + 1) * BPP); i++) {
				Log("info : checking button\n");
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
	for (u8 i = 0; i < fontButtons.size(); i++) {
		if(fontButtons[i]) delete fontButtons[i];
	}
	fontButtons.clear();
}

void App::FontDraw()
{
	FontDraw(true);
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
	for (u8 i = fontPage * 7; (i < fontButtons.size()) && (i < (fontPage + 1) * BPP); i++)
	{
		fontButtons[i]->Draw(ts->screenright, i == fontSelected);
	}
	buttonprefs.Label("Cancel");
	buttonprefs.Draw(screen, false);
	if(fontButtons.size() > (fontPage + 1) * BPP)
		buttonnext.Draw(ts->screenright, false);
	if(fontSelected > BPP)
		buttonprev.Draw(ts->screenright, false);

	// restore state.
	ts->SetStyle(style);
	ts->SetInvert(invert);
	ts->SetScreen(screen);
}

void App::FontNextPage()
{
	if((fontPage + 1) * BPP < fontButtons.size())
	{
		fontPage += 1;
		fontSelected = fontPage * BPP;
	}
}

void App::FontPreviousPage()
{
	if(fontPage > 0)
	{
		fontPage--;
		fontSelected = fontPage * BPP + (BPP-1);
	}
}

void App::FontButton()
{
	Log("progr: assigning font..\n");
	bool invert = ts->GetInvert();

	ts->SetScreen(ts->screenright);
	ts->SetInvert(false);
	ts->ClearScreen();
	ts->SetPen(ts->margin.left,PAGE_HEIGHT/2);
	ts->PrintString("[saving font...]");
	ts->SetInvert(invert);

	std::string path = fontdir;
	path.append(fontButtons[fontSelected]->GetLabel());
	if (mode == APP_MODE_PREFS_FONT)
		ts->SetFontFile(path.c_str(), TEXT_STYLE_NORMAL);
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

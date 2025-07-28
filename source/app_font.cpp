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

#include "main.h"
#include "parse.h"
#include "app.h"
#include "book.h"
#include "button.h"
#include "text.h"

#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)
#define BPP 7  // buttons per page

void App::FontInit()
{
	DIR *dp = opendir(fontdir.c_str());
	if (!dp)
	{
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
			b->Move(0, (fontButtons.size() % BPP) * b->GetHeight());
			b->Label(filename);
 			fontButtons.push_back(b);
		}
	}
	closedir(dp);
}

void App::FontHandleEvent()
{
	auto keys = keysDown();

	if (keys & (KEY_SELECT | KEY_B))
	{
		// Leave without changing the font
		mode = APP_MODE_PREFS;
		prefs_view_dirty = true;
	}
	else if (keys & key.up)
	{
		// Previous page
		if (fontSelected > 0) {
			if (fontSelected == fontPage * 7) {
				FontPreviousPage();
				FontDraw();
			} else {
				fontSelected--;
				font_view_dirty = true;
			}
		}
	}
	else if (keys & key.down)
	{
		// Next page
		if (fontSelected == fontPage * BPP + (BPP-1)) {
			FontNextPage();
		} else {
			fontSelected++;
		}
		font_view_dirty = true;
	}
	else if (keys & KEY_A)
	{
		FontButton();
	}
	else if (keys & KEY_TOUCH)
	{
		FontHandleTouchEvent();
	}
}

void App::FontHandleTouchEvent() {
	touchPosition coord = TouchRead();

	if(buttonprefs.EnclosesPoint(coord.py,coord.px)) {
		// Leave views
		mode = APP_MODE_PREFS;
		prefs_view_dirty = true;
	}
	else if(buttonnext.EnclosesPoint(coord.py,coord.px)){
		FontNextPage();
	}
	else if(buttonprev.EnclosesPoint(coord.py,coord.px)) {
		FontPreviousPage();
	}
	else
	{
		for(u8 i = fontPage * BPP;
			(i < fontButtons.size()) && (i < (fontPage + 1) * BPP);
			i++) {
			if (fontButtons[i]->EnclosesPoint(coord.py, coord.px))
			{
				fontSelected = i;
				char msg[4];
				sprintf(msg, "%d", fontSelected);
				PrintStatus(msg);
				FontButton();
				break;
			}
		}
	}
}

void App::FontDraw()
{
	// save state.
	bool invert = ts->GetInvert();
	u16* screen = ts->GetScreen();
	int style = ts->GetStyle();
	
	ts->SetInvert(false);
	ts->SetStyle(TEXT_STYLE_BROWSER);
	ts->ClearScreen();

	for (u8 i = fontPage * 7; (i < fontButtons.size()) && (i < (fontPage + 1) * BPP); i++)
	{
		fontButtons[i]->Draw(ts->screenright, i == fontSelected);
	}
	buttonprefs.Label("cancel");
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
	const char* filename = fontButtons[fontSelected]->GetLabel();
	if (filename == nullptr)
	{
		PrintStatus("error");
		return;
	}
	switch (mode) {
		case APP_MODE_PREFS_FONT:
		ts->SetFontFile(filename, TEXT_STYLE_REGULAR);
		PrefsRefreshButtonFont();
		break;

		case APP_MODE_PREFS_FONT_BOLD:
		ts->SetFontFile(filename, TEXT_STYLE_BOLD);
		PrefsRefreshButtonFontBold();
		break;

		case APP_MODE_PREFS_FONT_ITALIC:
		ts->SetFontFile(filename, TEXT_STYLE_ITALIC);
		PrefsRefreshButtonFontItalic();
		break;

		case APP_MODE_PREFS_FONT_BOLDITALIC:
		ts->SetFontFile(filename, TEXT_STYLE_BOLDITALIC);
		PrefsRefreshButtonFontBoldItalic();
		break;
	}

	// Trigger repagination
	ts->Init();
	
	prefs_view_dirty = true;
	mode = APP_MODE_PREFS;
	prefs->Write();
}

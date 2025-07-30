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

#include "main.h"
#include "parse.h"
#include "app.h"
#include "book.h"
#include "button.h"
#include "text.h"

#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)

void App::PrefsInit()
{	
	const std::vector<std::string> labels
	{
		"regular font",
		"bold font",
		"italic font",
		"bold italic font",
		"font size",
		"paragraph spacing",
		"screen orientation"
	};

	for (int i=0; i<PREFS_BUTTON_COUNT; i++)
	{
		prefsButtons[i].Init(ts);
		prefsButtons[i].SetStyle(BUTTON_STYLE_SETTING);
		prefsButtons[i].SetLabel1(labels[i]);
		PrefsRefreshButton(i);
		prefsButtons[i].Move(0, i * prefsButtons[i].GetHeight());
	}

	if (prefsSelected == -1) prefsSelected = PREFS_BUTTON_FONTSIZE;
}

void App::PrefsDraw()
{
	// save state.
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();
	u16* screen = ts->GetScreen();
	int style = ts->GetStyle();

	ts->SetScreen(ts->screenright);
	ts->SetInvert(false);

	for (u8 i = 0; i < PREFS_BUTTON_COUNT; i++)
	{
		prefsButtons[i].Draw(ts->screenright, i==prefsSelected);
	}

	if (strcmp(buttonprefs.GetLabel(), "  books"))
		buttonprefs.Label("  books");
	buttonprefs.Draw(ts->screenright);

	// restore state.
	ts->SetStyle(style);
	ts->SetInvert(invert);
	ts->SetPixelSize(size);
	ts->SetScreen(screen);

	prefs_view_dirty = false;
}

void App::PrefsHandleEvent()
{
	int keys = keysDown();
	
	if (keys & KEY_A)
	{
		PrefsHandlePress();
	}
	else if (keys & (KEY_SELECT | KEY_B))
	{
		buttonprefs.Label("settings");
		mode = APP_MODE_BROWSER;
		browser_draw();
	}
	else if (keys & (key.left | key.l))
	{
		if(prefsSelected < PREFS_BUTTON_COUNT - 1) {
			prefsSelected++;
			PrefsDraw();
		}
	}
	else if (keys & (key.right | key.r))
	{
		if(prefsSelected > 0) {
			prefsSelected--;
			PrefsDraw();
		}
	}
	else if (prefsSelected == PREFS_BUTTON_FONTSIZE && (keys & key.up))
	{
		PrefsDecreasePixelSize();
	}
	else if (prefsSelected == PREFS_BUTTON_FONTSIZE && (keys & key.down))
	{
		PrefsIncreasePixelSize();
	}
	else if (prefsSelected == PREFS_BUTTON_PARASPACING && (keys & key.up))
	{
		PrefsDecreaseParaspacing();
	}
	else if (prefsSelected == PREFS_BUTTON_PARASPACING && (keys & key.down))
	{
		PrefsIncreaseParaspacing();
	}
	else if (keys & KEY_START)
	{
		mode = APP_MODE_QUIT;
		prefs->Write();
	}
	else if (keys & KEY_TOUCH)
	{
		PrefsHandleTouch();
	}
}

void App::PrefsHandleTouch() {
	touchPosition coord = TouchRead();
	
	if (buttonprefs.EnclosesPoint(coord.py, coord.px)) {
		buttonprefs.Label("settings");
		mode = APP_MODE_BROWSER;
		browser_view_dirty = true;
		return;
	}

	for(u8 i = 0; i < PREFS_BUTTON_COUNT; i++) {
		if (prefsButtons[i].EnclosesPoint(coord.py, coord.px))
		{
			if (i != prefsSelected) {
				prefsSelected = i;
			}
			
			if (i == PREFS_BUTTON_FONTSIZE) {
				if (coord.py > 2 + 188 / 2) {
					PrefsIncreasePixelSize();
				} else {
					PrefsDecreasePixelSize();
				}
			} else if (i == PREFS_BUTTON_PARASPACING) {
				if (coord.py > 2 + 188 / 2) {
					PrefsIncreaseParaspacing();
				} else {
					PrefsDecreaseParaspacing();
				}
			} else if (i == PREFS_BUTTON_ORIENTATION) {
				PrefsFlipOrientation();
			} else {
				PrefsHandlePress();
			}
			
			break;
		}
	}

	if (prefs_view_dirty) PrefsDraw();
}

void App::PrefsIncreasePixelSize()
{
	if (ts->pixelsize < 24) {
		ts->pixelsize++;
		PrefsRefreshButton(PREFS_BUTTON_FONTSIZE);
		PrefsDraw();
		bookcurrent = NULL;
		prefs->Write();
	}
}

void App::PrefsDecreasePixelSize()
{
	if (ts->pixelsize > 4) {
		ts->pixelsize--;
		PrefsRefreshButton(PREFS_BUTTON_FONTSIZE);
		PrefsDraw();
		bookcurrent = NULL;
		prefs->Write();
	}
}

void App::PrefsIncreaseParaspacing()
{
	if (paraspacing < 2) {
		paraspacing++;
		PrefsRefreshButton(PREFS_BUTTON_PARASPACING);
		PrefsDraw();
		bookcurrent = NULL;
		prefs->Write();
	}
}

void App::PrefsDecreaseParaspacing()
{
	if (paraspacing > 0) {
		paraspacing--;
		PrefsRefreshButton(PREFS_BUTTON_PARASPACING);
		PrefsDraw();
		bookcurrent = NULL;
		prefs->Write();
	}
}

void App::PrefsFlipOrientation()
{
	SetOrientation(!orientation);
	PrefsRefreshButton(PREFS_BUTTON_ORIENTATION);
	PrefsDraw();
	prefs->Write();
}

void App::PrefsRefreshButton(int index) {
	char msg[64];
	switch (index) {
		case PREFS_BUTTON_FONT:
			prefsButtons[PREFS_BUTTON_FONT].SetLabel2(
				ts->GetFontName(TEXT_STYLE_REGULAR)
			);
		case PREFS_BUTTON_FONT_BOLD:
			prefsButtons[PREFS_BUTTON_FONT_BOLD].SetLabel2(
				ts->GetFontName(TEXT_STYLE_BOLD)
			);
		case PREFS_BUTTON_FONT_BOLDITALIC:
			prefsButtons[PREFS_BUTTON_FONT_BOLDITALIC].SetLabel2(
				ts->GetFontName(TEXT_STYLE_BOLDITALIC)
			);
			break;
		case PREFS_BUTTON_FONT_ITALIC:
			prefsButtons[PREFS_BUTTON_FONT_ITALIC].SetLabel2(
				ts->GetFontName(TEXT_STYLE_ITALIC)
			);
			break;
		case PREFS_BUTTON_FONTSIZE:
			sprintf(msg, "                        < %d >  ", ts->GetPixelSize());
			prefsButtons[PREFS_BUTTON_FONTSIZE].SetLabel2(std::string(msg));
			break;
		case PREFS_BUTTON_PARASPACING:
			sprintf(msg, "                         < %d >  ", paraspacing);
			prefsButtons[PREFS_BUTTON_PARASPACING].SetLabel2(std::string(msg));
			break;
		case PREFS_BUTTON_ORIENTATION:
			prefsButtons[PREFS_BUTTON_ORIENTATION].SetLabel2(
				orientation 
				? std::string("Turned Right")
				: std::string("Turned Left")
			);
		break;
	}
}

void App::PrefsHandlePress()
{
	//! Go to font view or flip orientation.

	if (prefsSelected == PREFS_BUTTON_ORIENTATION) {
		PrefsFlipOrientation();
		prefs_view_dirty = true;
		return;
	}

	if (prefsSelected == PREFS_BUTTON_FONT) {
		mode = APP_MODE_PREFS_FONT;
	} else if (prefsSelected == PREFS_BUTTON_FONT_BOLD) {
		mode = APP_MODE_PREFS_FONT_BOLD;
	} else if (prefsSelected == PREFS_BUTTON_FONT_ITALIC) {
		mode = APP_MODE_PREFS_FONT_ITALIC;
	} else if (prefsSelected == PREFS_BUTTON_FONT_BOLDITALIC) {
		mode = APP_MODE_PREFS_FONT_BOLDITALIC;
	}
	FontInit();
	font_view_dirty = true;
}

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

#include "types.h"
#include "main.h"
#include "parse.h"
#include "App.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"

#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)

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
	sprintf((char*)msg, "Regular Font:\n %s", ts->GetFontFile(TEXT_STYLE_NORMAL).c_str());
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
	if (paraspacing == 0)
		sprintf((char*)msg, "Paragraph Spacing:\n    [ %d >", paraspacing);
	else if (paraspacing == 255)
		sprintf((char*)msg, "Paragraph Spacing:\n    < %d ]", paraspacing);
	else
		sprintf((char*)msg, "Paragraph Spacing:\n    < %d >", paraspacing);
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
		browser_draw();
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
		u8 regionprev[2];
		regionprev[0] = 0;
		regionprev[1] = 16;

		if(!orientation)
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
			browser_draw();
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
	if (paraspacing < 255) {
		paraspacing++;
		PrefsRefreshButtonParaspacing();
		PrefsDraw();
		bookcurrent = NULL;
		prefs->Write();
	}
}

void App::PrefsDecreaseParaspacing()
{
	if (paraspacing > 0) {
		paraspacing--;
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

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
	prefsButtonFont.Init(ts);
	prefsButtonFont.Move(0, PREFS_BUTTON_FONT * prefsButtonFont.GetHeight());
	PrefsRefreshButtonFont();
	prefsButtons[PREFS_BUTTON_FONT] = &prefsButtonFont;
	
	prefsButtonFontBold.Init(ts);
	prefsButtonFontBold.Move(0, PREFS_BUTTON_FONT_BOLD * prefsButtonFontBold.GetHeight());
	PrefsRefreshButtonFontBold();
	prefsButtons[PREFS_BUTTON_FONT_BOLD] = &prefsButtonFontBold;
		
	prefsButtonFontItalic.Init(ts);
	prefsButtonFontItalic.Move(0, PREFS_BUTTON_FONT_ITALIC * prefsButtonFontItalic.GetHeight());
	PrefsRefreshButtonFontItalic();
	prefsButtons[PREFS_BUTTON_FONT_ITALIC] = &prefsButtonFontItalic;

	prefsButtonFontBoldItalic.Init(ts);
	prefsButtonFontBoldItalic.Move(0, PREFS_BUTTON_FONT_BOLDITALIC * prefsButtonFontBoldItalic.GetHeight());
	PrefsRefreshButtonFontBoldItalic();
	prefsButtons[PREFS_BUTTON_FONT_BOLDITALIC] = &prefsButtonFontBoldItalic;
	
	prefsButtonFontSize.Init(ts);
	prefsButtonFontSize.Move(0, PREFS_BUTTON_FONTSIZE * prefsButtonFontSize.GetHeight());
	PrefsRefreshButtonFontSize();
	prefsButtons[PREFS_BUTTON_FONTSIZE] = &prefsButtonFontSize;
	
	prefsButtonParaspacing.Init(ts);
	prefsButtonParaspacing.Move(0, PREFS_BUTTON_PARASPACING * prefsButtonParaspacing.GetHeight());
	PrefsRefreshButtonParaspacing();
	prefsButtons[PREFS_BUTTON_PARASPACING] = &prefsButtonParaspacing;
	
	prefsButtonOrientation.Init(ts);
	prefsButtonOrientation.Move(0, PREFS_BUTTON_ORIENTATION * prefsButtonOrientation.GetHeight());
	PrefsRefreshButtonOrientation();
	prefsButtons[PREFS_BUTTON_ORIENTATION] = &prefsButtonOrientation;

	for (auto button : prefsButtons)
		button->SetStyle(BUTTON_STYLE_SETTING);

	prefsSelected = 0;
}

void App::PrefsRefreshButtonFont()
{
	prefsButtonFont.SetLabel1(std::string("regular font"));
	prefsButtonFont.SetLabel2(ts->GetFontName(TEXT_STYLE_REGULAR));
}

void App::PrefsRefreshButtonFontBold()
{
	prefsButtonFontBold.SetLabel1(std::string("bold font"));
	prefsButtonFontBold.SetLabel2(ts->GetFontName(TEXT_STYLE_BOLD));
}

void App::PrefsRefreshButtonFontItalic()
{
	prefsButtonFontItalic.SetLabel1(std::string("italic font"));
	prefsButtonFontItalic.SetLabel2(ts->GetFontName(TEXT_STYLE_ITALIC));
}

void App::PrefsRefreshButtonFontBoldItalic()
{
	prefsButtonFontBoldItalic.SetLabel1(std::string("bold italic font"));
	prefsButtonFontBoldItalic.SetLabel2(ts->GetFontName(TEXT_STYLE_BOLDITALIC));
}

void App::PrefsRefreshButtonFontSize()
{
	prefsButtonFontSize.SetLabel1(std::string("font size"));
	char msg[64];
	if (ts->GetPixelSize() == 1)
		sprintf(msg, "    [ %d >", ts->GetPixelSize());
	else if (ts->GetPixelSize() == 255)
		sprintf(msg, "    < %d ]", ts->GetPixelSize());
	else
		sprintf(msg, "    < %d >", ts->GetPixelSize());
	prefsButtonFontSize.SetLabel2(std::string(msg));
}

void App::PrefsRefreshButtonParaspacing()
{
	prefsButtonParaspacing.SetLabel1(std::string("paragraph spacing"));
	char msg[64];
	if (paraspacing == 0)
		sprintf(msg, "    [ %d >", paraspacing);
	else if (paraspacing == 255)
		sprintf(msg, "    < %d ]", paraspacing);
	else
		sprintf(msg, "    < %d >", paraspacing);
	prefsButtonParaspacing.SetLabel2(std::string(msg));
}

void App::PrefsRefreshButtonOrientation()
{
	prefsButtonOrientation.SetLabel1(std::string("screen orientation"));
	prefsButtonOrientation.SetLabel2(
		orientation 
			? std::string("ABXY on Bottom")
			: std::string("D-Pad on Bottom")
	);
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
		prefsButtons[i]->Draw(ts->screenright, i==prefsSelected);
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
		PrefsButton();
	}

	else if (keys & KEY_X)
	{
		CycleBrightness();
		prefs->Write();
	}

	else if (keys & (KEY_START | KEY_SELECT | KEY_B))
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

	else if (prefsSelected == PREFS_BUTTON_FONTSIZE && (keys & key.up)) {
		PrefsDecreasePixelSize();
	} 

	else if (prefsSelected == PREFS_BUTTON_FONTSIZE && (keys & key.down)) {
		PrefsIncreasePixelSize();
	} 

	else if (prefsSelected == PREFS_BUTTON_PARASPACING && (keys & key.up)) {
		PrefsDecreaseParaspacing();
	} 

	else if (prefsSelected == PREFS_BUTTON_PARASPACING && (keys & key.down)) {
		PrefsIncreaseParaspacing();
	}

	else if (keys & KEY_TOUCH)
	{
		touchPosition touch;
		touchRead(&touch);
		touchPosition coord;

		if(!orientation)
		{
			coord.px = 256 - touch.px;
			coord.py = 192 - touch.py;
		} else {
			coord.px = touch.px;
			coord.py = touch.py;
		}
		
		if (buttonprefs.EnclosesPoint(coord.py, coord.px)) {
			buttonprefs.Label("settings");
			mode = APP_MODE_BROWSER;
			browser_view_dirty = true;
			return;
		}

		for(u8 i = 0; i < PREFS_BUTTON_COUNT; i++) {
			if (prefsButtons[i]->EnclosesPoint(coord.py, coord.px))
			{
				if (i != prefsSelected) {
					prefsSelected = i;
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
				} else if (i == PREFS_BUTTON_ORIENTATION) {
					PrefsFlipOrientation();
				} else {
					PrefsButton();
				}
				
				break;
			}
		}
	}

	if (prefs_view_dirty) PrefsDraw();
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

void App::PrefsFlipOrientation()
{
	orientation = !orientation;
	prefs->Write();

	prefs_view_dirty = true;
}

void App::PrefsButton()
{
	if (prefsSelected == PREFS_BUTTON_FONT) {
		mode = APP_MODE_PREFS_FONT;
	} else if (prefsSelected == PREFS_BUTTON_FONT_BOLD) {
		mode = APP_MODE_PREFS_FONT_BOLD;
	} else if (prefsSelected == PREFS_BUTTON_FONT_ITALIC) {
		mode = APP_MODE_PREFS_FONT_ITALIC;
	} else if (prefsSelected == PREFS_BUTTON_FONT_BOLDITALIC) {
		mode = APP_MODE_PREFS_FONT_BOLDITALIC;
	} else if (prefsSelected == PREFS_BUTTON_ORIENTATION) {
		PrefsFlipOrientation();
		return;
	}

	ts->SetScreen(ts->screenright);
	ts->ClearScreen();
	FontInit();
	FontDraw();
}

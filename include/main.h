/*
 Copyright (C) 2007-2020 Ray Haleblian
 
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
 
 To contact the copyright holder: ray@haleblian.com
 */

#ifndef _main_h_
#define _main_h_

#include <ft2build.h>
#include FT_FREETYPE_H
#include "nds.h"
#include "splash.h"

#define MARGINLEFT 12
#define MARGINRIGHT 12
#define MARGINTOP 10
#define MARGINBOTTOM 16
#define LINESPACING 0
#define PARASPACING 0
#define PAGE_HEIGHT SCREEN_WIDTH
#define PAGE_WIDTH SCREEN_HEIGHT
#define BUFSIZE 1024*128
#define SPLASH_LEFT 28
#define SPLASH_TOP 44

#define LOGFILEPATH "dslibris.log"
#define PREFSPATH "dslibris.xml"

#define FONTREGULARFILE "LiberationSerif-Regular.ttf"
#define FONTBOLDFILE "LiberationSerif-Bold.ttf"
#define FONTITALICFILE "LiberationSerif-Italic.ttf"
#define FONTBOLDITALICFILE "LiberationSerif-BoldItalic.ttf"
#define FONTBROWSERFILE "LiberationSans-Regular.ttf"
#define FONTSPLASHFILE "LiberationSans-Regular.ttf"

int halt(int vblanks = -1);
int halt(const char *msg, int vblanks = -1);

//! For drawing splash screen.
int getSize(u8 *source, u16 *dest, u32 arg);
u8 readByte(u8 *source);
void drawstack(u16 *screen);

#endif

/*
 Copyright (C) 2007-2009 Ray Haleblian
 
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

/*!
 \mainpage
 
 Welcome to the documentation for dslibris, a book reader for Nintendo DS.
 
 For notes on prerequisites, building and debugging,
 see <a href="../../README.txt">README.txt</a> in the distribution. 
 
 For notes on installation, see
 <a href="../../INSTALL.txt">INSTALL.txt</a> in the distribution. 
 
 The project homepage is 
 <a href="http://sourceforge.net/projects/ndslibris">
 http://sourceforge.net/projects/ndslibris</a> .
 
 Thanks to the developers of devkitPro, DSGUI, FreeType, and in the homebrew
 community for the components lovingly borrowed in order to make this project
 work.
 
 This software is licensed under the GPL;
 all rights and responsibilities relating to use, reuse or distribution
 of this software follow the license.
 This software does not, nor is intended to, violate or circumvent
 any product or content copyright, licensing, or rules of use.
 Respect the copyright of any content you consider using
 with this or any other software. Enjoy!
 */

#include <ft2build.h>
#include FT_FREETYPE_H

#define MARGINLEFT 12
#define MARGINRIGHT 12
#define MARGINTOP 10
#define MARGINBOTTOM 16
#define LINESPACING 0
#define PARASPACING 0
#define PAGE_HEIGHT SCREEN_WIDTH
#define PAGE_WIDTH SCREEN_HEIGHT
#define BUFSIZE 1024*16		//! For XML parsing temp buffer (bytes).
#define PAGEBUFSIZE 2048	//! For storing UTF-8 for a page (bytes).
#define MAXPAGES 2048
#define SPLASH_LEFT 28
#define SPLASH_TOP 44

#define FONTDIR "/font/"
#define BOOKDIR "/book/"
#define PREFSPATH "dslibris.xml"
#define FONTFILEPATH "/font/LiberationSerif-Regular.ttf"
#define FONTBOLDFILEPATH "/font/LiberationSerif-Bold.ttf"
#define FONTITALICFILEPATH "/font/LiberationSerif-Italic.ttf"
#define FONTBROWSERFILEPATH "/font/LiberationSans-Regular.ttf"
#define FONTSPLASHFILEPATH "/font/LiberationSans-Regular.ttf"
#define LOGFILEPATH "dslibris.log"

//! Initial entry point and (possibly obsolete) data structures.

void splash();

//! Book (obsolete?)
typedef struct book_s {
	char *filename;
	char *title;
	char *author;
	s16 position;
} book_t;


//! Page data allocated as array in instances of a book.
typedef struct page_s {
	//! First char index in the cooked book.
	//! Use this for locating bookmarks.
	uint32 startchar; 
	u16 length;
	u8 *buf;   //! allocated per-page at parse time, to exact length.
} page_t;

//! Data passed to expat callbacks when parsing preferences.
typedef struct {
	char filename[32];
	u16 position;
} prefsdata_t;

void page_init(page_t *page);

#endif

#pragma once

#include <sys/param.h>

#define APP_BROWSER_BUTTON_COUNT 7
#define APP_LOGFILE "dslibris.log"
#define APP_MODE_BOOK 0
#define APP_MODE_BROWSER 1
#define APP_MODE_PREFS 2
#define APP_MODE_PREFS_FONT 3
#define APP_MODE_PREFS_FONT_BOLD 4
#define APP_MODE_PREFS_FONT_ITALIC 5
#define APP_URL "http://github.com/rhaleblian/dslibris"
#define BOOK_BUTTON_COUNT 7
#define BOOKDIR "Book"
#define PARSEBUFSIZE 1024*64
#define PREFSPATH "dslibris.xml"

#define FONTREGULARFILE "LiberationSerif-Regular.ttf"
#define FONTBOLDFILE "LiberationSerif-Bold.ttf"
#define FONTITALICFILE "LiberationSerif-Italic.ttf"
#define FONTBROWSERFILE "LiberationSans-Regular.ttf"
#define FONTSPLASHFILE "LiberationSans-Regular.ttf"

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

#define FONTDIR "Font"

//! Reference: FreeType2 online documentation
#define EMTOPIXEL (float)(POINTSIZE * DPI/72.0)

//! Reference: http://www.displaymate.com/psp_ds_shootout.htm
#define DPI 110

#define PREFS_ERROR_PARSE 254
#define PREFS_ERROR_MISSING 255

#define TEXT_BOLD_ON 2
#define TEXT_BOLD_OFF 3
#define TEXT_ITALIC_ON 4
#define TEXT_ITALIC_OFF 5
#define TEXT_STYLE_REGULAR (u8)0
#define TEXT_STYLE_BOLD (u8)1
#define TEXT_STYLE_ITALIC (u8)2
#define TEXT_STYLE_BROWSER (u8)3
#define TEXT_STYLE_SPLASH (u8)4

#define CACHESIZE 512
#define PIXELSIZE 12

#define PAGEBUFSIZE 2048

#define LOGFILEPATH "dslibris.log"

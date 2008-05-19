#ifndef _main_h_
#define _main_h_

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
#define BUFSIZE 1024
#define PAGEBUFSIZE 2048
#define MAXPAGES 2048
#define MAXBOOKS 32
#define SPLASH_LEFT (MARGINLEFT)
#define SPLASH_TOP (MARGINTOP+32)

#define FONTDIR "/font/"
#define BOOKDIR "/book/"
#define PREFSPATH "dslibris.xml"
#define FONTFILEPATH "/font/dslibris.ttf"
#define FONTBOLDFILEPATH "/font/dslibrisb.ttf"
#define FONTITALICFILEPATH "/font/dslibrisi.ttf"
#define LOGFILEPATH "dslibris.log"

/** watch out for struct alignment here? **/
typedef struct book_s {
	char *filename;
	char *title;
	char *author;
	s16 position;
} book_t;

typedef struct page_s {
	u16 length;
	u8 *buf;   /** allocated per-page at parse time, to exact length. **/
} page_t;

typedef struct {
	char filename[32];
	u16 position;
} prefsdata_t;

void page_init(page_t *page);

#endif

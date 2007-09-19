#ifndef _main_h_
#define _main_h_

#include <ft2build.h>
#include FT_FREETYPE_H

#define MARGINLEFT 12
#define MARGINRIGHT 12
#define MARGINTOP 10
#define MARGINBOTTOM 16
#define LINESPACING 2
#define PARASPACING 0
#define PIXELSIZE 10
#define DPI 72	/** probably not true for a DS - measure it **/
#define EMTOPIXEL (float)(POINTSIZE * DPI/72.0)
#define PAGE_HEIGHT SCREEN_WIDTH
#define PAGE_WIDTH SCREEN_HEIGHT
#define BUFSIZE 1024
#define PAGEBUFSIZE 2048
#define MAXPAGES 2048
#define MAXBOOKS 7

/** watch out for struct alignment here? **/
typedef struct book_s {
  char filename[32];
  char title[30];
  u16 position;
} book_t;

typedef struct page_s {
  u16 length; 
  u8 *buf;   /** allocated per-page at parse time, to exact length. **/
} page_t;

typedef enum {NONE,HTML,HEAD,TITLE,BODY,PRE} context_t;

typedef struct {
  context_t stack[16];
  u8 stacksize;
  book_t *book;
  page_t *page;
  FT_Vector pen;
} parsedata_t;

typedef struct {
  char filename[32];
  u16 position;
} prefsdata_t;

void page_init(page_t *page);

#endif

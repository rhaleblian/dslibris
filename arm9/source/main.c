/**----------------------------------------------------------------------------
   $Id: main.c,v 1.17 2007/09/16 00:14:48 rhaleblian Exp $
   dslibris - an ebook reader for Nintendo DS
   -------------------------------------------------------------------------**/

#include <nds.h>
#include <fat.h>
#include <dswifi9.h>
#include <nds/registers_alt.h>
#include <nds/reload.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <errno.h>

#include <expat.h>
//#include <jpeglib.h>
//#include <png.h>

#include "main.h"
#include "parse.h"
#include "ui.h"
#include "wifi.h"

#include "stupidWifiDebug.h"
#include "debug_tcp.h"
#include "debug_stub.h"

#include "all_gfx.c"
#include "all_gfx.h"

#define APP_VERSION "0.2.2"
#define APP_URL "http://rhaleblian.wordpress.com"

u16 *screen0, *screen1, *fb;

book_t books[MAXBOOKS];
u8 bookcount, bookcurrent;

page_t pages[MAXPAGES];
u8 pagebuf[PAGEBUFSIZE];
u16 pagecount, pagecurrent;

button_t buttons[16];
char msg[128];
bool invert = false;
parsedata_t parsedata;

void initConsole(void);

void parse_init(parsedata_t *data);
void parse_printerror(XML_Parser p);
bool parse_pagefeed(parsedata_t *data, page_t *page);

void book_init(book_t *book);
u8   book_parse(XML_Parser p, char *filebuf);
void book_parsetitle(char *filename);

void bookmark_read(void);
void bookmark_write(void);

void browser_init(void);
void browser_draw(void);

void splash_draw(void);

void page_init(page_t *page);
void page_clear(void);
void page_clearone(u16 *screen);
void page_clearinvert(u16 *screen);
void page_draw(page_t *page);
void page_drawsolid(u8 r, u8 g, u8 b);
void page_drawmargins(void);
u8   page_getjustifyspacing(page_t *page, u16 i);

void prefs_start_hndl(void *data,
		      const XML_Char *name, 
		      const XML_Char **attr);
bool prefs_read(XML_Parser p);
void prefs_write(void);

void statusmsg(const char *msg);

/*---------------------------------------------------------------------------*/

void spin(void) { while(true) swiWaitForVBlank(); }

int main(void)
{
  bool browseractive = false;
  char filebuf[BUFSIZE];

  powerSET(POWER_LCD|POWER_2D_A|POWER_2D_B);
  defaultExceptionHandler();  /** guru meditation! */

  irqInit();
  irqEnable(IRQ_VBLANK);
  irqEnable(IRQ_VCOUNT);

  /** cw rotation for both screens **/
  s16 s = SIN[-128 & 0x1FF] >> 4;
  s16 c = COS[-128 & 0x1FF] >> 4;

  /** bring up the startup console.
      sub bg 0 will be used to print text. **/
  videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);	
  vramSetBankC(VRAM_C_SUB_BG);
  SUB_BG0_CR = BG_MAP_BASE(31);
  {
    u32 i;
    for(i=0;i<255;i++)
      BG_PALETTE_SUB[i] = RGB15(0,0,0);
    BG_PALETTE_SUB[255] = RGB15(15,15,15);
  }
  consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31),
		     (u16*)CHAR_BASE_BLOCK_SUB(0), 16);
  printf("startup console        [  OK  ]\n");
  swiWaitForVBlank();

  printf("media filesystem       ");
  if(!fatInitDefault()) { printf("[ FAIL ]\n"); spin(); }
  else printf("[  OK  ]\n");
  swiWaitForVBlank();

  printf("typesetter             ");
  if(tsInitDefault()) { printf("[ FAIL ]\n"); spin(); }
  else printf("[  OK  ]\n");
  swiWaitForVBlank();

  printf("book scan              ");
  bookcount = 0;
  bookcurrent = 0;
  /** assemble library by indexing all XHTML/XML files
      in the current directory.
      TODO recursive book search **/
  char dirname[16] = ".";
  DIR_ITER *dp = diropen(dirname);
  if(!dp) { printf("[ FAIL ]\n"); spin(); }
  char filename[32];
  while(!dirnext(dp, filename, NULL) && bookcount != MAXBOOKS) {  
    char *c;
    for(c=filename;c!=filename+strlen(filename) && *c!='.';c++);
    if(!stricmp(".xht",c) || !stricmp(".xhtml",c)) {
      book_t *book = &(books[bookcount]);
      book_init(book);			
      sprintf(book->filename,"%s/%s",dirname,filename);
      book_parsetitle(filename);
      bookcount++;
    }
  }
  dirclose(dp);
  if(!bookcount) { printf("[ FAIL ]\n"); spin(); }
  else printf("[  OK  ]\n");
  swiWaitForVBlank();
  browser_init();

  printf("expat XML parser       ");
  XML_Parser p = XML_ParserCreate(NULL);
  if(!p) { printf("[ FAIL ]\n"); spin(); }
  XML_SetUnknownEncodingHandler(p,unknown_hndl,NULL);
  parse_init(&parsedata);
  printf("[  OK  ]\n");
  swiWaitForVBlank();

  /** initialize book view. **/

  BACKGROUND.control[3] = BG_BMP16_256x256 | BG_BMP_BASE(0);
  BG3_XDX = c;
  BG3_XDY = -s;
  BG3_YDX = s;
  BG3_YDY = c;
  BG3_CX = 0 << 8;
  BG3_CY = 256 << 8;
  videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
  vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
  fb = screen1 = (u16*)BG_BMP_RAM(0);
  
  BACKGROUND_SUB.control[3] = BG_BMP16_256x256 | BG_BMP_BASE(0);
  SUB_BG3_XDX = c;
  SUB_BG3_XDY = -s;
  SUB_BG3_YDX = s;
  SUB_BG3_YDY = c;
  SUB_BG3_CX = 0 << 8;
  SUB_BG3_CY = 256 << 8;
 
  videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
  vramSetBankC(VRAM_C_SUB_BG_0x06200000);
  screen0 = (u16*)BG_BMP_RAM_SUB(0);
  page_clear();
  fb = screen0;

  if(prefs_read(p)) {
    swiWaitForVBlank();
    page_clear();

    pagecount = 0;
    pagecurrent = 0;
    page_t *page = &(pages[pagecurrent]);
    page_init(page);

    page_clearinvert(screen0);
    page_clearinvert(screen1);
    tsSetInvert(true);
    statusmsg("[loading...]");
    tsSetInvert(false);
    book_parse(p, filebuf);

    fb = screen0;
    tsInitPen();

    pagecurrent = 0;
    bookmark_read();
    page = &(pages[pagecurrent]);
    page_draw(page);
    browseractive = false;
  } else {
    tsInitPen();
    browser_draw();
    splash_draw();
    fb = screen0;
    browseractive = true;
  }
  swiWaitForVBlank();

  touchPosition touch;

  while(true) {    
    scanKeys();

    if(keysDown() & KEY_TOUCH) {
      touch = touchReadXY();
      //      fb[touch.px + touch.py * 256] = rand();
      if(pagecurrent < pagecount) {
	pagecurrent++;
	page_draw(&pages[pagecurrent]);
      }
    }

    if(browseractive) {

      if(keysDown() & KEY_A) {
	/** parse the selected book. **/

	swiWaitForVBlank();
	page_clear();

	pagecount = 0;
	pagecurrent = 0;
	page_t *page = &(pages[pagecurrent]);
	page_init(page);

	page_clearinvert(screen0);
	page_clearinvert(screen1);
	tsSetInvert(true);
	statusmsg("[loading...]");
	tsSetInvert(false);
	if(!book_parse(p, filebuf)) {
	  
	  fb = screen0;
	  tsInitPen();

	  pagecurrent = 0;
	  bookmark_read();
	  page = &(pages[pagecurrent]);
	  page_draw(page);
	  prefs_write();
	  browseractive = false;
	} else {
	  browser_draw();
	}
      }
      
      if(keysDown() & KEY_B) {
	browseractive = false;
	page_draw(&pages[pagecurrent]);
      }
      
      if(keysDown() & (KEY_LEFT|KEY_L)) {
	if(bookcurrent < bookcount-1) {
	  bookcurrent++;
	  browser_draw();
	  fb = screen0;
	}
      }
      
      if(keysDown() & (KEY_RIGHT|KEY_R)) {
	if(bookcurrent > 0) {
	  bookcurrent--;
	  browser_draw();
	  fb = screen0;
	}
      }
      
      if(keysDown() & KEY_SELECT) {
	browseractive = false;
	page_draw(&(pages[pagecurrent]));
      }
      
    } else {
      
      if(keysDown() & (KEY_A|KEY_DOWN|KEY_R)) {
	if(pagecurrent < pagecount) {
	  pagecurrent++;
	  page_draw(&pages[pagecurrent]);
	  prefs_write();
	  bookmark_write();
	}
      }
      
      if(keysDown() & (KEY_B|KEY_UP|KEY_L)) {
	if(pagecurrent > 0) {
	  pagecurrent--;
	  page_draw(&pages[pagecurrent]);
	  prefs_write();
	  bookmark_write();
	}
      }

      if(keysDown() & KEY_SELECT) {
	bookmark_write();
	prefs_write();
	browseractive = true;
	browser_draw();	
	splash_draw();
	fb = screen0;
      }
    }
    swiWaitForVBlank();
  }

  XML_ParserFree(p);
  exit(0);
}

void initConsole(void) {
  /** bring up the startup console **/  
  videoSetMode(MODE_0_2D);	
  videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);	
  /** sub bg 0 will be used to print text **/
  vramSetBankC(VRAM_C_SUB_BG);
  /** set palette - green on black **/
  SUB_BG0_CR = BG_MAP_BASE(31);
  int i;
  for(i=0;i<256;i++) BG_PALETTE_SUB[i] = RGB15(1,3,1);
  BG_PALETTE_SUB[255] = RGB15(15,31,15);
  consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31),
		     (u16*)CHAR_BASE_BLOCK_SUB(0), 16);
}

void browser_init(void) {
  u8 book;
  for(book=0;book<bookcount;book++) {
    button_init(&buttons[book]);
    button_move(&buttons[book],0,book*32);
    if(strlen((char *)books[book].title))
      strcpy((char *)buttons[book].text,(char*)books[book].title);
    else
      strcpy((char *)buttons[book].text,(char*)books[book].filename);
  }
}

void browser_draw(void) {
  fb = screen1;
  page_drawsolid(0,0,0);
  u8 i;
  for(i=0;i<bookcount;i++) {
    if(i==bookcurrent) button_draw(&buttons[i],fb,true);
    else button_draw(&buttons[i],fb,false);
  }
}

void book_parsetitle(char *filename) {
  char path[32];
  strncpy(path,(char*)filename,32);
  FILE *fp = fopen(path,"r");
  if(fp) {
    XML_Parser p = XML_ParserCreate(NULL);
    XML_SetUserData(p, &parsedata);
    parse_init(&parsedata);
    XML_SetElementHandler(p, start_hndl, end_hndl);
    XML_SetCharacterDataHandler(p, title_hndl);
    XML_SetProcessingInstructionHandler(p, proc_hndl);
    XML_Char filebuf[BUFSIZE];
    while(true) {
      int bytes_read = fread(filebuf, 1, BUFSIZE, fp);
      if(XML_Parse(p, filebuf, bytes_read,bytes_read == 0)) break;
      if(!bytes_read) break;
    }
    XML_ParserFree(p);
    fclose(fp);
  }
}

void parse_printerror(XML_Parser p)
{
  sprintf(msg,"expat: [%s]\n",XML_ErrorString(XML_GetErrorCode(p)));
  tsString(msg);
  sprintf(msg,"expat: [%d:%d] : %d\n",
	  (int)XML_GetCurrentLineNumber(p),
	  (int)XML_GetCurrentColumnNumber(p),
	  (int)XML_GetCurrentByteIndex(p));
  tsString(msg);
}

void book_init(book_t *book)
{
  strcpy((char *)book->filename,"");
  strcpy((char *)book->title,"");
}

u8 book_parse(XML_Parser p, char *filebuf)
{
  char path[64];
  sprintf(path,"%s",books[bookcurrent].filename);
  printf(path);
  printf("\n");
  FILE *fp = fopen(path,"r");
  if(!fp) {
    sprintf(msg,"[cannot open %s]\n",path);
    tsString(msg);
    return(1);
  }

  XML_ParserReset(p,NULL);
  parse_init(&parsedata);
  XML_SetUserData(p, &parsedata);
  XML_SetDefaultHandler(p, default_hndl);
  XML_SetElementHandler(p, start_hndl, end_hndl);
  XML_SetCharacterDataHandler(p, char_hndl);
  XML_SetProcessingInstructionHandler(p, proc_hndl);

  enum XML_Status status;
  while(true) {
    int bytes_read = fread(filebuf, 1, BUFSIZE, fp);
    status = XML_Parse(p, filebuf, bytes_read, (bytes_read == 0));
    if(status == XML_STATUS_ERROR) {
      parse_printerror(p);
      break;
    }
    if(bytes_read == 0) break;
  }
  
  fclose(fp);
  return(0);
}

void page_init(page_t *page) {
  page->length = 0;
  page->buf = NULL;
}

u8 page_getjustifyspacing(page_t *page, u16 i) {
  /** full justification. get line advance, count spaces,
      and insert more space in spaces to reach margin.
      returns amount of space to add per-character. **/

  u8 spaces = 0;
  u8 advance = 0;
  u8 j,k;

  /* walk through leading spaces */
  for(j=i;j<page->length && page->buf[j]==' ';j++);

  /* find the end of line */
  for(j=i;j<page->length && page->buf[j]!='\n';j++) {
    u16 c = page->buf[j];
    advance += tsAdvance(c);
    
    if(page->buf[j] == ' ') spaces++;
  }
  
  /* walk back through trailing spaces */
  for(k=j;k>0 && page->buf[k]==' ';k--) spaces--;

  if(spaces) 
    return((u8)((float)((PAGE_WIDTH-MARGINRIGHT-MARGINLEFT) - advance) 
		 / (float)spaces));
  else return(0);
}

void page_drawsolid(u8 r, u8 g, u8 b) {
  u16 color = RGB15(r,g,b) | BIT(15);
  int i;
  for(i=0;i<PAGE_HEIGHT*PAGE_HEIGHT;i++) fb[i] =color;
}


void page_drawmargins(void) {
  int x, y;
  u16 color = RGB15(30,30,30) | BIT(15);
  for(y=0;y<PAGE_HEIGHT;y++) {
    fb[y*PAGE_HEIGHT + MARGINLEFT] = color;
    fb[y*PAGE_HEIGHT + PAGE_WIDTH-MARGINRIGHT] = color;
  }
  for(x=0;x<PAGE_HEIGHT;x++) {
    fb[MARGINTOP*PAGE_HEIGHT + x] = color;
    fb[(PAGE_HEIGHT-MARGINBOTTOM)*PAGE_HEIGHT + x] = color;
  }
}

void page_clear(void) {
  u16 *savefb = fb;
  fb = screen0;
  page_drawsolid(31,31,31);
  fb = screen1;
  page_drawsolid(31,31,31);
  fb = savefb;
}

void page_clearone(u16 *screen) {
  u16 *savefb = fb;
  fb = screen;
  page_drawsolid(31,31,31);
  fb = savefb;
}

void page_clearinvert(u16 *screen) {
  u16 *savefb = fb;
  fb = screen;
  page_drawsolid(0,0,0);
  fb = savefb;
}

#define SPLASH_LEFT (MARGINLEFT+28)
#define SPLASH_TOP (MARGINTOP+96)

void splash_draw(void) {
  fb = screen0;
  tsSetPixelSize(36);
  tsSetInvert(true);
  page_clearinvert(screen0);
  tsSetPen(SPLASH_LEFT,SPLASH_TOP);
  tsString("dslibris");
  tsSetPixelSize(0);
  tsSetPen(SPLASH_LEFT,tsGetPenY()+tsGetHeight());
  tsString("an ebook reader");
  tsSetPen(SPLASH_LEFT,tsGetPenY()+tsGetHeight());
  tsString("for Nintendo DS");
  tsSetPen(SPLASH_LEFT,tsGetPenY()+tsGetHeight());
  tsString(APP_VERSION);
  tsNewLine();
  tsSetPen(SPLASH_LEFT,tsGetPenY()+tsGetHeight());
  sprintf(msg,"%d books\n", bookcount);
  tsString(msg);
  statusmsg(APP_URL);
  tsSetInvert(false);
}

void page_draw(page_t *page) {
  page_clear();
  fb = screen0;
  tsInitPen();
  u8 spacing = page_getjustifyspacing(page,0);
  u16 i=0; 
  while(i<page->length) {
    u16 c = page->buf[i];
    if(c == '\n') {
      spacing = page_getjustifyspacing(page,i+1);
      tsNewLine();
      i++;
    } else {
      if(c > 127) i+=ucs((char*)&(page->buf[i]),&c);
      else i++;
      tsChar(c);
      //if(c == ' ') au16 x,y; tsGetPen(&x,&y); x += spacing; tsSetPen(x,y);
    }
  }

  fb = screen1;
  int offset = 170 * (pagecurrent / (float)pagecount);
  tsSetPen(MARGINLEFT+offset,250);
  sprintf((char*)msg,"[%d]",pagecurrent+1);
  tsString(msg);
}

void bookmark_filename(book_t *book, char *filename) {
  int i;
  for(i=0;book->filename[i] != '.';i++);
  strcpy(filename,"");
  strncat(filename,book->filename,i);
  strcat(filename,".bkm");  
}

void bookmark_write(void) {
  book_t *book = &(books[bookcurrent]);
  char filename[64];
  bookmark_filename(book,filename);
  FILE *fp = fopen(filename,"w");
  if(fp) {
    fprintf(fp,"%d\n",(int)pagecurrent);
    fclose(fp);
  }
}

void bookmark_read(void) {
  book_t *book = &(books[bookcurrent]);
  char filename[64];
  bookmark_filename(book,filename);
  FILE *fp = fopen(filename,"r");
  if(fp) {
    int pagesave;
    fscanf(fp, "%d", &pagesave);
    pagecurrent = (u16)pagesave;
    fclose(fp);
  }
}

void statusmsg(const char *msg) {
  u16 *savefb = fb;
  fb = screen0;
  u16 x,y;
  tsGetPen(&x,&y);

  tsSetPen(MARGINLEFT,SCREEN_WIDTH-MARGINBOTTOM);
  tsString("                                                     ");
  tsSetPen(MARGINLEFT,SCREEN_WIDTH-MARGINBOTTOM);
  tsString(msg);

  tsSetPen(x,y);
  fb = savefb;
}

/** parse.c **/

int min(int x, int y) { if(y < x) return y; else return x; }

bool iswhitespace(u8 c) {
  switch(c) {
  case ' ':
  case '\t':
  case '\n':
  case '\r':
    return true;
    break;    
  default:
    return false;
    break;
  }
}

void parse_init(parsedata_t *data) {
  data->stacksize = 0;
  data->book = NULL;
  data->page = NULL;
  data->pen.x = MARGINLEFT;
  data->pen.y = MARGINTOP;
}

void parse_push(parsedata_t *data, context_t context) {
  data->stack[data->stacksize++] = context;
}

context_t parse_pop(parsedata_t *data) {
  if(data->stacksize) data->stacksize--;
  return data->stack[data->stacksize];
}

bool parse_in(parsedata_t *data, context_t context) {
  u8 i;
  for(i=0;i<data->stacksize;i++) {
    if(data->stack[i] == context) return true;
  }
  return false;
}

bool parse_pagefeed(parsedata_t *data, page_t *page) {
  int pagedone;

  /** we are at the end of one of the facing pages. **/
  if(fb == screen1) {

    /** we left the right page, save chars into this page. **/
    if(!page->buf) {
      page->buf = malloc(page->length * sizeof(u8));
      if(!page->buf) {
	tsString("[out of memory]\n");
	spin();
      }
    }     
    memcpy(page->buf,pagebuf,page->length * sizeof(u8));
    fb = screen0;
    pagedone = true;

  } else {
    fb = screen1;
    pagedone = false;
  }
  data->pen.x = MARGINLEFT;
  data->pen.y = MARGINTOP + tsGetHeight();
  return pagedone;
}

#if 0
void lidsleep(void) {
  __asm("mcr p15,0,r0,c7,c0,4");
  __asm("mov r0, r0");
  __asm("BX lr"); 
}

void lidsleep2(void) { 
  /* if lid 'key' */
  if (keysDown() & BIT(7)) {
    /* hinge is closed */
    /* set only key irq for waking up */
    unsigned long oldIE = REG_IE ;
    REG_IE = IRQ_KEYS ;
    *(volatile unsigned short *)0x04000132 = BIT(14) | 255 ; 
    /* any of the inner keys might irq */
    /* power off everything not needed */
    powerOFF(POWER_LCD) ;
    /* set system into sleep */
    lidsleep() ;
    /* wait a bit until returning power */
    while (REG_VCOUNT!=0) ;
    while (REG_VCOUNT==0) ;
    while (REG_VCOUNT!=0) ;
    /* power on again */
    powerON(POWER_LCD) ;
    /* set up old irqs again */
    REG_IE = oldIE ;
  }
}
#endif

void prefs_start_hndl(void *data,
		      const XML_Char *name, 
		      const XML_Char **attr) {
  if(!stricmp(name,"bookmark")) {
    prefsdata_t *ud = (prefsdata_t *)data;
    u8 i;
    for(i=0;attr[i];i+=2) {
      tsString(" ");
      tsString((char*)attr[i]);
      tsString("=");
      tsString((char*)attr[i+1]);
      if(!strcmp(attr[i],"file")) strcpy(ud->filename, attr[i+1]);
      if(!strcmp(attr[i],"position")) ud->position = atoi(attr[i+1]);
    }
  }
}

/** side effect - sets bookcurrent to index of bookmarked book. **/

bool prefs_read(XML_Parser p) {
  FILE *fp = fopen("dslibris.xml","r");
  if(!fp) return false;
  prefsdata_t ud;
  strcpy(ud.filename,"");
  ud.position = 0;

  XML_ParserReset(p, NULL);
  XML_SetStartElementHandler(p, prefs_start_hndl);
  XML_SetUserData(p, (void *)&ud);
  while(true) {
    swiWaitForVBlank();
    void *buff = XML_GetBuffer(p, 64);
    int bytes_read = fread(buff, sizeof(char), 64, fp);
    XML_ParseBuffer(p, bytes_read, bytes_read == 0);
    if(bytes_read == 0) break;
  }
  fclose(fp);

  if(!strlen(ud.filename)) return false;

  /** handle bookmark. find a match by filename. **/
  int i;
  for(i=0;i<bookcount;i++) {
    if(!stricmp(books[i].filename,ud.filename)) {
      //      books[i].position = ud.position;
      bookcurrent = i;
      return true;
    }
  }
  return false;
}

void prefs_write(void) {
  FILE* fp = fopen("dslibris.xml","w");
  fprintf(fp,"<dslibris>\n");
  fprintf(fp, "<bookmark file=\"%s\" position=\"%d\" />",
	  books[bookcurrent].filename, pagecurrent);
  fprintf(fp, "\n</dslibris>\n");
  fclose(fp);
}

int unknown_hndl(void *encodingHandlerData,
		 const XML_Char *name,
		 XML_Encoding *info) {
  return(XML_STATUS_ERROR);
}

void default_hndl(void *data, const XML_Char *s, int len) {
  parsedata_t *p = (parsedata_t *)data;
  if(s[0] == '&') {
    page_t *page = &(pages[pagecurrent]);
    
    /** an iso-8859-1 character code. */
    if(!strnicmp(s,"&nbsp;",5)) {
      pagebuf[page->length++] = ' ';
      p->pen.x += tsAdvance(' ');
      return;
    }

    /** if it's numeric, decode. */
    int code=0;
    sscanf(s,"&#%d;",&code);
    if(code) {
      if(code>=128 && code<=2047) {
	pagebuf[page->length++] = 192 + (code/64);
	pagebuf[page->length++] = 128 + (code%64);
      }
	
      if(code>=2048 && code<=65535) {
	pagebuf[page->length++] = 224 + (code/4096);
	pagebuf[page->length++] = 128 + ((code/64)%64);
	pagebuf[page->length++] = 128 + (code%64);
      }

      p->pen.x += tsAdvance(code);
    }
  }
}  /* End default_hndl */

void start_hndl(void *data, const char *el, const char **attr) {
  if(!stricmp(el,"html")) parse_push(data,HTML);
  if(!stricmp(el,"body")) parse_push(data,BODY);
  if(!stricmp(el,"title")) parse_push(data,TITLE);
  if(!stricmp(el,"head")) parse_push(data,HEAD);
  if(!stricmp(el,"pre")) parse_push(data,PRE);
}  /* End of start_hndl */

void title_hndl(void *data, const char *txt, int txtlen) {
  if(parse_in(&parsedata,TITLE)) {
    if(txtlen > 30) {
      strncpy((char *)books[bookcount].title,txt, 27);
      strcat((char*)books[bookcount].title, "...");
    } else {
      strncpy((char *)books[bookcount].title,txt, min(txtlen,30));
    }
  }
}

void char_hndl(void *data, const XML_Char *txt, int txtlen) {
  /** flow text on the fly, into page data structure. **/

  if(!parse_in(data,BODY)) return;

  parsedata_t *p = (parsedata_t *)data;

  int i=0;
  u8 advance=0;
  static bool linebegan=false;
  page_t *page = &(pages[pagecurrent]);
	
  /** starting a new page? **/
  if(page->length == 0) {
    linebegan = false;
    p->pen.x = MARGINLEFT;
    p->pen.y = MARGINTOP + tsGetHeight();  
  }
  
  while(i<txtlen) {
    if(txt[i] == '\r') { i++; continue; }

    if(iswhitespace(txt[i])) {
      if(parse_in(data,PRE) && txt[i] == '\n') {
	pagebuf[page->length++] = txt[i];
	p->pen.x += tsAdvance((u16)txt[i]);
	p->pen.y += (tsGetHeight() + LINESPACING);
	if(p->pen.y > (PAGE_HEIGHT-MARGINBOTTOM)) {
	  if(parse_pagefeed(data,page)) {
	    page++;
	    page_init(page);
	    pagecurrent++;
	    pagecount++;
	  }
	  linebegan = false;
	}

      } else {
	if(linebegan) {
	  pagebuf[page->length++] = ' ';
	  p->pen.x += tsAdvance((u16)' ');
	}

      }
      i++;

    } else {
      linebegan = true;
      int j;
      advance = 0;
      u8 bytes = 1;
      for(j=i;(j<txtlen) && (!iswhitespace(txt[j]));j+=bytes) {

	/** set type until the end of the next word.
	    account for UTF-8 characters when advancing. **/
	u16 code;
	if(txt[j] > 127) 
	  bytes = ucs((char*)&(txt[j]),&code);
	else {
	  code = txt[j];
	  bytes = 1;
	}
	advance += tsAdvance(code);
      }

      /** reflow. if we overrun the margin, insert a break. **/
      
      int overrun = (p->pen.x + advance) - (PAGE_WIDTH-MARGINRIGHT);
      if(overrun > 0) {
	pagebuf[page->length] = '\n';
	page->length++;
	p->pen.x = MARGINLEFT;
	p->pen.y += (tsGetHeight() + LINESPACING);

	if(p->pen.y > (PAGE_HEIGHT-MARGINBOTTOM)) {
	  if(parse_pagefeed(data,page)) {
	    page++;
	    page_init(page);
	    pagecurrent++;
	    pagecount++;
	  }
	  linebegan = false;
	}
      }
      
      /** append this word to the page. to save space,
	  chars will stay UTF-8 until they are rendered. **/
      for(;i<j;i++) {
	if(iswhitespace(txt[i])) {
	  if(linebegan) {
	    pagebuf[page->length] = ' ';
	    page->length++;
	  }
	} else {
	  linebegan = true;
	  pagebuf[page->length] = txt[i];
	  page->length++;
	}
      }
      p->pen.x += advance;
    }
  }
}  /* End char_hndl */

void end_hndl(void *data, const char *el) {
  page_t *page = &pages[pagecurrent];
  parsedata_t *p = (parsedata_t *)data;
  if(
     !stricmp(el,"br")
     || !stricmp(el,"p")
     || !stricmp(el,"h1")
     || !stricmp(el,"h2")
     || !stricmp(el,"h3")
     || !stricmp(el,"h4")
     || !stricmp(el,"hr")
     ) {
    pagebuf[page->length] = '\n';
    page->length++;
    p->pen.x = MARGINLEFT;
    p->pen.y += tsGetHeight() + LINESPACING;
    if( !stricmp(el,"p")) {
      pagebuf[page->length] = '\n';
      page->length++;
      p->pen.y += tsGetHeight() + LINESPACING;
    }
    if(p->pen.y > (PAGE_HEIGHT-MARGINBOTTOM)) {
      if(fb == screen1) {
	fb = screen0;
	if(!page->buf)
	  page->buf = malloc(page->length * sizeof(char));
	strncpy((char*)page->buf,(char *)pagebuf,page->length);
	page++;
	page_init(page);
	pagecurrent++;
	pagecount++;
      } else {
	fb = screen1;
      }
      p->pen.x = MARGINLEFT;
      p->pen.y = MARGINTOP + tsGetHeight();
    }
  }
  if(!stricmp(el,"body")) {
    if(!page->buf) {
      page->buf = malloc(page->length * sizeof(char));
      if(!page->buf) tsString("[alloc error]\n");
    }
    strncpy((char*)page->buf,(char*)pagebuf,page->length);
    parse_pop(data);
  }
  if(!(stricmp(el,"title") && stricmp(el,"head") 
       && stricmp(el,"pre") && stricmp(el,"html"))) {
    parse_pop(&parsedata);
  }
  
}  /* End of end_hndl */

void proc_hndl(void *data, const char *target, const char *pidata) {
}  /* End proc_hndl */

/*
void user_error_ptr() {}
void user_error_fn() {}
void user_warning_fn() {}

#define ERROR 1
u8 splash(void)
{
  char file_name[32] = "dslibris.png";
  FILE *fp = fopen(file_name, "rb");

  png_structp png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)user_error_ptr,
     user_error_fn, user_warning_fn);
  if (!png_ptr)
    return (1);

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
      png_destroy_read_struct(&png_ptr,
			      (png_infopp)NULL, (png_infopp)NULL);
      return (2);
    }
  
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
    {
      png_destroy_read_struct(&png_ptr, &info_ptr,
			      (png_infopp)NULL);
      return (3);
    }
  
  fclose(fp);
  return(0);
}
*/


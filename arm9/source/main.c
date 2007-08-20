/* $Id: main.c,v 1.6 2007/08/19 05:31:16 rhaleblian Exp $
   dslibris - an ebook reader for Nintendo DS
   $Log: main.c,v $
   Revision 1.6  2007/08/19 05:31:16  rhaleblian
   XHTML filenames need to conform to 8.3 to work under Ubuntu.

   Revision 1.5  2007/08/18 03:55:09  rhaleblian
   text now renders again. added title search within found books.

   Revision 1.4  2007/08/15 04:59:12  rhaleblian
   bookmarks barely work. filenames cannot exceed 7 chars.

   Revision 1.3  2007/08/14 05:52:42  rhaleblian
   small tweaks to ndstool and build rules.

   Revision 1.2  2007/08/13 04:26:03  rhaleblian
   Suppressed arm7 code, which causes memory and pc stomping.
   Added banner logo and title.
   START reveals book list.
*/

/*#include <nds.h>*/
#include <PA9.h>
#include <fat.h>
#include <dswifi9.h>
#include <expat.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <errno.h>

#include "main.h"
#include "parse.h"
#include "font.h"
#include "ui.h"
#include "wifi.h"

#include "stupidWifiDebug.h"
#include "debug_tcp.h"
#include "debug_stub.h"

/** libnds r20 removed BACKGROUND et al? the below is for OS X **/

#ifndef BACKGROUND
typedef struct {
  u16 xdx;
  u16 xdy;
  u16 ydx;
  u16 ydy;
  u32 centerX;
  u32 centerY;    
} bg_rotation;  // need this here?
typedef struct {
  u16 control[4];
  bg_scroll scroll[4];
  bg_rotation bg2_rotation;
  bg_rotation bg3_rotation;
} bg_attribute;
#define BACKGROUND           (*((bg_attribute *)0x04000008))
#define BACKGROUND_SUB       (*((bg_attribute *)0x04001008))
#endif

u16 *screen0, *screen1, *fb;

book_t books[MAXBOOKS];
u8 bookcount, bookcurrent;

page_t pages[MAXPAGES];
u8 pagebuf[PAGEBUFSIZE];
u16 pagecount, pagecurrent;

context_t context;
button_t buttons[16];
char msg[128];

/** pen for computin reflow at parse time, not drawing;
    font.c owns the drawing pen. **/
FT_Vector ppen;		

#if 0
struct parsedata {
  context_t context;
  FT_Vector pen;
  page_t *page;
  u8 *pagebuf;
};
#endif

u8 getjustifyspacing(page_t *page, u16 i);

void book_init(book_t *book);
void bookreadposition(void);
void bookwriteposition(void);

void drawsolid(u8 r, u8 g, u8 b);
void drawmargins(void);
void drawblankpages(void);
void drawbrowser(void);
void drawpages(page_t *page);

void makebrowser(void);
void searchfortitle(char *filename);
void printerror(XML_Parser p);
u8 bookparse(XML_Parser p, char *filebuf);

void spin(void) { while(true) swiWaitForVBlank(); }

/*---------------------------------------------------------------------------*/

int main(void) {
  bool browseractive = false;
  char filebuf[BUFSIZE];

  powerSET(POWER_LCD|POWER_2D_A|POWER_2D_B);
  defaultExceptionHandler();
  irqInit();

#if WIFIDEBUG
  wifiDebugInit();
  debugHalt();
#endif
  
  irqEnable(IRQ_VBLANK);

  /** bring up the startup console **/

  videoSetMode(0);	//not using the main screen
  videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);	
  //sub bg 0 will be used to print text
  vramSetBankC(VRAM_C_SUB_BG);
  SUB_BG0_CR = BG_MAP_BASE(31);
  {
    u32 i;
    for(i=0;i<256;i++)
      BG_PALETTE_SUB[i] = RGB15(0,0,0);
    BG_PALETTE_SUB[255] = RGB15(15,31,15);
  }
  consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);
  printf("console                    [OK]\n");

  printf("filesystem               ");
  if(!fatInitDefault()) {
    printf("[FAIL]\n");
  } else
    printf("  [OK]\n");

  printf("typesetter               ");
  if(tsInitDefault()) {
    printf("[FAIL]\n");
  } else
    printf("  [OK]\n");
  swiWaitForVBlank();

#if WIFI
  wifiInit();
  printf("wifi                     ");
  printf("  [OK]\n");
#endif

  bookcount = 0;
  bookcurrent = 0;

  /** assemble library, aka XHTML/XML files in the current directory. **/
  
  printf("book library             ");
  DIR_ITER *dp = diropen(".");
  if(!dp) {
    printf("[FAIL]\n");
    spin();
  }
  char filename[32];
  while(!dirnext(dp, filename, NULL)) {
    if((bookcount < MAXBOOKS)
       && (!stricmp(".xht", filename + (strlen(filename)-4)))
       ) {
      book_t *book = &(books[bookcount]);
      book_init(book);			
      strncpy(book->filename,filename,32);
      searchfortitle(filename);
      bookcount++;
    }
  }
  dirclose(dp);
  printf("  [OK]\n");
  swiWaitForVBlank();

  printf("XML Parser               ");
  XML_Parser p = XML_ParserCreate(NULL);
  if(p) printf("  [OK]\n");
  else {
    printf("[FAIL]\n");
    spin();
  }
  XML_SetUnknownEncodingHandler(p,unknown_hndl,NULL);
  swiWaitForVBlank();

  u8 i;
  for(i=0;i<60;i++) swiWaitForVBlank();

  /** initialize the book view. **/

  /** rotation for both screens **/
  s16 s = SIN[-128 & 0x1FF] >> 4;
  s16 c = COS[-128 & 0x1FF] >> 4;

  BACKGROUND.control[3] = BG_BMP16_256x256 | BG_BMP_BASE(0);
  BG3_XDX = c;
  BG3_XDY = -s;
  BG3_YDX = s;
  BG3_YDY = c;
  BG3_CX = 0 << 8;
  BG3_CY = 256 << 8;
  
  BACKGROUND_SUB.control[3] = BG_BMP16_256x256 | BG_BMP_BASE(0);
  SUB_BG3_XDX = c;
  SUB_BG3_XDY = -s;
  SUB_BG3_YDX = s;
  SUB_BG3_YDY = c;
  SUB_BG3_CX = 0 << 8;
  SUB_BG3_CY = 256 << 8;
  
  videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
  videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
  vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
  vramSetBankC(VRAM_C_SUB_BG_0x06200000);
  screen1 = (u16*)BG_BMP_RAM(0);
  screen0 = (u16*)BG_BMP_RAM_SUB(0);
  drawblankpages();
  fb = screen0;
  tsString("[welcome to dslibris]\n");
  swiWaitForVBlank();
  tsDump();
  tsChar(198); /** &AElig; **/
  swiWaitForVBlank();

  makebrowser();
  drawbrowser();
  fb = screen0;
  swiWaitForVBlank();
  browseractive = true;
  
  while(true) {    
    scanKeys();

    if(browseractive) {
      
      if(keysDown() & KEY_A) {
	/** parse the selected book. **/

	swiWaitForVBlank();
	drawblankpages();

	pagecount = 0;
	pagecurrent = 0;
	page_t *page = &(pages[pagecurrent]);
	pageinit(page);

	if(bookparse(p, filebuf)) break;

	fb = screen0;
	tsInitPen();

	pagecurrent = 0;
	bookreadposition();
	page = &(pages[pagecurrent]);
	drawpages(page);
	browseractive = false;
      }
      
      if(keysDown() & KEY_B) {
	browseractive = false;
	drawpages(&pages[pagecurrent]);
      }
      
      if(keysDown() & (KEY_LEFT|KEY_L)) {
	if(bookcurrent < bookcount-1) {
	  bookcurrent++;
	  drawbrowser();
	  fb = screen0;
	}
      }
      
      if(keysDown() & (KEY_RIGHT|KEY_R)) {
	if(bookcurrent > 0) {
	  bookcurrent--;
	  drawbrowser();
	  fb = screen0;
	}
      }

      if(keysDown() & KEY_SELECT) {
	browseractive = false;
	drawpages(&(pages[pagecurrent]));
      }
      
    } else {
      
      if(keysDown() & (KEY_A|KEY_DOWN|KEY_R)) {
	if(pagecurrent < pagecount) {
	  pagecurrent++;
	  drawpages(&pages[pagecurrent]);
	}
      }
      
      if(keysDown() & (KEY_B|KEY_UP|KEY_L)) {
	if(pagecurrent > 0) {
	  pagecurrent--;
	  drawpages(&pages[pagecurrent]);
	}
      }

      if(keysDown() & KEY_RIGHT) {
	bookwriteposition();
      }

      if(keysDown() & KEY_SELECT) {
	bookwriteposition();
	browseractive = true;
	drawbrowser();
	fb = screen0;
      }

      if(keysDown() & KEY_START) {
	swiSoftReset();
      }
    }
      swiWaitForVBlank();
  }

  XML_ParserFree(p);
  exit(0);
}




#if 0
  {
    int initdata = Wifi_Init(NULL);
    tsString("[wifi: arm9 initialized]\n");
    while(!Wifi_CheckInit());
    tsString("[wifi: arm7 initialized]\n");
    Wifi_EnableWifi();
    Wifi_SetChannel(11);
    Wifi_AccessPoint *apmatch;
    Wifi_AccessPoint apdata;
    Wifi_AutoConnect();
    while(true) {
      int status = Wifi_AssocStatus();
      if(status == ASSOCSTATUS_ASSOCIATED) {
	tsString("[connected]\n");
	break;
      }
      if(status == ASSOCSTATUS_CANNOTCONNECT) {
	tsString("[cannot connect]\n"); 
	break;
      }
    }
    int addr = Wifi_GetIP();
    sprintf(msg,"[ip: %d\n]",addr);
    tsString(msg);

    stupidWifiDebugSendInt(1);
    stupidWifiDebugSendString("hello\n",6);
  }
#endif

#if 0
void wifiDebugInit(void)
{
  /** wifi debugging **/
  struct tcp_debug_comms_init_data init_data = {
    .port = 30000
  };
  if ( !init_debug( &tcpCommsIf_debug, &init_data)) {
    iprintf("Failed to initialise the debugger stub - cannot continue\n");
    drawsolid(31,0,0);
    while(true) swiWaitForVBlank();
  }
}
#endif

void makebrowser(void) {
  u8 book;
  for(book=0;book<bookcount;book++) {
    initbutton(&buttons[book]);
    movebutton(&buttons[book],0,book*32);
    if(strlen((char *)books[book].title))
      strcpy((char *)buttons[book].text,(char*)books[book].title);
    else
      strcpy((char *)buttons[book].text,(char*)books[book].filename);
  }
}

void searchfortitle(char *filename) {
  char path[32];
  strncpy(path,(char*)filename,32);
  FILE *fp = fopen(path,"r");
  if(fp) {
    XML_Parser p = XML_ParserCreate(NULL);
    XML_UseParserAsHandlerArg(p);
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


void printerror(XML_Parser p) {
  sprintf(msg,"expat: [%s]\n",XML_ErrorString(XML_GetErrorCode(p)));
  tsString(msg);
  sprintf(msg,"expat: [%d:%d] : %d\n",
	  (int)XML_GetCurrentLineNumber(p),
	  (int)XML_GetCurrentColumnNumber(p),
	  (int)XML_GetCurrentByteIndex(p));
  tsString(msg);
}

u8 bookparse(XML_Parser p, char *filebuf) {
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
  XML_UseParserAsHandlerArg(p);
  XML_SetDefaultHandler(p, default_hndl);
  XML_SetElementHandler(p, start_hndl, end_hndl);
  XML_SetCharacterDataHandler(p, char_hndl);
  XML_SetProcessingInstructionHandler(p, proc_hndl);

  enum XML_Status status;
  while(true) {
    int bytes_read = fread(filebuf, 1, BUFSIZE, fp);
    status = XML_Parse(p, filebuf, bytes_read, (bytes_read == 0));
    if(status == XML_STATUS_ERROR) {
      printerror(p);
      break;
    }
    if(bytes_read == 0) break;
  }
  
  fclose(fp);
  return(0);
}



void book_init(book_t *book) {
  strcpy((char *)book->filename,"");
  strcpy((char *)book->title,"");
}

void pageinit(page_t *page) {
  page->length = 0;
  page->buf = NULL;
}

u8 getjustifyspacing(page_t *page, u16 i) {
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

void drawsolid(u8 r, u8 g, u8 b) {
  u16 color = RGB15(r,g,b) | BIT(15);
  int i;
  for(i=0;i<PAGE_HEIGHT*PAGE_HEIGHT;i++) fb[i] =color;
}

void drawmargins(void) {
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

void drawblankpages(void) {
  u16 *savefb = fb;
  fb = screen0;
  drawsolid(31,31,31);
  fb = screen1;
  drawsolid(31,31,31);
  fb = savefb;
}

void drawbrowser(void) {
  fb = screen1;
  drawsolid(31,31,31);
  u8 i;
  for(i=0;i<bookcount;i++) {
    if(i==bookcurrent) drawbutton(&buttons[i],fb,true);
    else drawbutton(&buttons[i],fb,false);
  }
}

void drawpages(page_t *page) {
  drawblankpages();
  fb = screen0;
  tsInitPen();
#ifdef JUSTIFY
  u8 spacing = getjustifyspacing(page,0);
#endif
  u16 i=0; 
  while(i<page->length) {
    u16 c = page->buf[i];
    if(c == '\n') {
      tsStartNewLine();
#ifdef JUSTIFY
      if(i < page->length-1) spacing = getjustifyspacing(page,i+1);
#endif
      i++;
    } else {
      if(c > 127) i+=utf8(&(page->buf[i]),&c);
      else i++;
      tsChar(c);
#ifdef JUSTIFY
      if(c == ' ') {
	u16 x,y;
	tsGetPen(&x,&y);
	x += spacing;
	tsSetPen(x,y);
      }
#endif
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

void bookwriteposition(void) {
  book_t *book = &(books[bookcurrent]);
  char filename[64];
  bookmark_filename(book,filename);
  FILE *fp = fopen(filename,"w");
  if(fp) {
    fprintf(fp,"%d\n",(int)pagecurrent);
    fclose(fp);
  }
}

void bookreadposition(void) {
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

void statusmsg(char *msg) {
  u16 *savefb = fb;
  fb = screen1;
  u16 x,y;
  tsGetPen(&x,&y);

  tsSetPen(MARGINLEFT,MARGINBOTTOM);
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

int unknown_hndl(void *encodingHandlerData,
		 const XML_Char *name,
		 XML_Encoding *info) {
  return(XML_STATUS_ERROR);
}

void default_hndl(void *data, const XML_Char *s, int len) {
  if(s[0] == '&') {
    /* an iso-8859-1 character code. if it's numeric, decode. */
    page_t *page = &(pages[pagecurrent]);
    int code=0;
    fscanf(s,"&#%d;",&code);
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

      ppen.x += tsAdvance(code);
    }
  }
}  /* End default_hndl */

void start_hndl(void *data, const char *el, const char **attr) {
  if(!stricmp(el,"html")) context = HTML;
  if(!stricmp(el,"body")) context = BODY;
  if(!stricmp(el,"title")) context = TITLE;
  if(!stricmp(el,"head")) context = HEAD;
}  /* End of start_hndl */

void title_hndl(void *data, const char *txt, int txtlen) {
  if(context != TITLE) return;

  if(txtlen > 30) {
    strncpy((char *)books[bookcount].title,txt, 27);
    strcat((char*)books[bookcount].title, "...");
  } else {
    strncpy((char *)books[bookcount].title,txt, min(txtlen,30));
  }
}

void char_hndl(void *data, const XML_Char *txt, int txtlen) {
  /** flow text on the fly, into page data structure. **/

  if(context != BODY) return;

  int i=0;
  u8 advance=0;
  static bool linebegan=false;
  page_t *page = &(pages[pagecurrent]);
	
  /** starting a new page? **/
  if(page->length == 0) {
    linebegan = false;
    ppen.x = MARGINLEFT;
    ppen.y = MARGINTOP + tsGetHeight();  
  }
  
  while(i<txtlen) {
    if(txt[i] == '\r') {
      i++;

    } else if(iswhitespace(txt[i])) {
      if(linebegan) {
	pagebuf[page->length] = ' ';
	page->length++;
	ppen.x += tsAdvance((u16)' ');
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
	  bytes = utf8((unsigned char*)&(txt[j]),&code);
	else {
	  code = txt[j];
	  bytes = 1;
	}
	advance += tsAdvance(code);
      }

      /** reflow. if we overrun the margin, insert a break. **/

      int overrun = (ppen.x + advance) - (PAGE_WIDTH-MARGINRIGHT);
      if(overrun > 0) {
	pagebuf[page->length] = '\n';
	page->length++;
	ppen.x = MARGINLEFT;
	ppen.y += (tsGetHeight() + LINESPACING);
	if(ppen.y > (PAGE_HEIGHT-MARGINBOTTOM)) {

	  /** we are at the end of one of the facing pages. **/
	  if(fb == screen1) {

	    /** we left the right page, save chars into this page. **/
	    if(!page->buf) {
	      page->buf = malloc(page->length * sizeof(u8));
	      if(!page->buf) {
		tsString("[page alloc error]\n");
		spin();
	      }
	    }
	    memcpy(page->buf,pagebuf,page->length * sizeof(u8));
	    page++;
	    pageinit(page);
	    fb = screen0;
	    pagecurrent++;
	    pagecount++;

	  } else {
	    fb = screen1;
	  }
	  ppen.x = MARGINLEFT;
	  ppen.y = MARGINTOP + tsGetHeight();
	}
	linebegan = false;
      }
			
      /** append this word to the page. to save space,
	  chars will stay UTF-8 until they are drawn. **/
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
      ppen.x += advance;
    }
  }
}  /* End char_hndl */



void end_hndl(void *data, const char *el) {
  page_t *page = &pages[pagecurrent];
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
    ppen.x = MARGINLEFT;
    ppen.y += tsGetHeight() + LINESPACING;
    if(ppen.y > (PAGE_HEIGHT-MARGINBOTTOM)) {
      if(fb == screen1) {
	fb = screen0;
	if(!page->buf)
	  page->buf = malloc(page->length * sizeof(char));
	strncpy((char*)page->buf,(char *)pagebuf,page->length);
	page++;
	pageinit(page);
	pagecurrent++;
	pagecount++;
      } else {
	fb = screen1;
      }
      ppen.x = MARGINLEFT;
      ppen.y = MARGINTOP + tsGetHeight();
    }
  }
  if(!stricmp(el,"html")) context = NONE;
  if(!stricmp(el,"body")) {
    if(!page->buf) {
      page->buf = malloc(page->length * sizeof(char));
      if(!page->buf) tsString("[alloc error]\n");
    }
    strncpy((char*)page->buf,(char*)pagebuf,page->length);
    context = HTML;
  }
  if(!stricmp(el,"title")) context = HEAD;
  if(!stricmp(el,"head")) context = HTML;
}  /* End of end_hndl */

void proc_hndl(void *data, const char *target, const char *pidata) {
}  /* End proc_hndl */

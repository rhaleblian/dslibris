/* dslibris - an ebook reader for Nintendo DS */

#include <nds.h>
#include <fat.h>
#include <dswifi9.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <errno.h>

#include "../../local/include/expat.h"
#include "main.h"
#include "font.h"
#include "ui.h"

#include "debug_tcp.h"
#include "debug_stub.h"

#define BUFSIZE 1024*32
#define PAGEBUFSIZE 4096
#define MAXPAGES 2048
#define MAXBOOKS 8

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

/** pen for computin reflow at parse time, not drawing;
    font.c owns the drawing pen. **/
FT_Vector ppen;		

u16 *screen0, *screen1, *fb;
u8 msg[128];

/** watch out for struct alignment here! 64 is working. **/
typedef struct book_s {
  u8 filename[32];
  u8 title[16];
  u8 author[16];
} book_t;
book_t books[MAXBOOKS];
u8 bookcount;
u8 bookcurrent;

typedef struct page_s {
  u16 length; 
  u8 *buf;   /** allocated per-page at parse time, to exact length. **/
} page_t;
page_t pages[MAXPAGES];
u8 pagebuf[PAGEBUFSIZE];
u16 pagecount;
u16 pagecurrent;

typedef enum {NONE,HTML,HEAD,TITLE,BODY} context_t;
context_t context;

/*struct parsedata {
  context_t context;
  FT_Vector pen;
  page_t *page;
  u8 *pagebuf;
  };*/

button_t buttons[16];

void initbook(book_t *book) {
  strcpy((char *)book->filename,"");
  strcpy((char *)book->title,"");
}

void pageinit(page_t *page) {
  page->length = 0;
  page->buf = NULL;
}

void drawsolid(int r, int g, int b) {
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

u8 getjustifyspacing(page_t *page, u16 i) {
  // full justification. get line advance, count spaces,
  // and insert more space in spaces to reach margin.
  // returns amount of space to add per-character.

  u8 spaces = 0;
  u8 advance = 0;
  u8 j,k;

  // walk through leading spaces
  for(j=i;j<page->length && page->buf[j]==' ';j++);

  // find the end of line
  for(j=i;j<page->length && page->buf[j]!='\n';j++) {
    u16 c = page->buf[j];
    advance += tsAdvance(c);
    
    if(page->buf[j] == ' ') spaces++;
  }
  
  // walk back through trailing spaces
  for(k=j;k>0 && page->buf[k]==' ';k--) spaces--;

  if(spaces) 
    return((u8)((float)((PAGE_WIDTH-MARGINRIGHT-MARGINLEFT) - advance) 
		 / (float)spaces));
  else return(0);
}

void drawbrowser(void) {
  fb = screen1;
  drawsolid(31,31,31);
  u8 i;
  for(i=0;i<bookcount;i++) {
    if(i==bookcurrent) drawbutton(&buttons[i],fb,1);
    else drawbutton(&buttons[i],fb,0);
  }
}

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

void default_hndl(void *data, const char *s, int len) {
}  /* End default_hndl */

void start_hndl(void *data, const char *el, const char **attr) {
  if(!stricmp(el,"html")) context = HTML;
  if(!stricmp(el,"body")) context = BODY;
  if(!stricmp(el,"title")) context = TITLE;
  if(!stricmp(el,"head")) context = HEAD;
}  /* End of start_hndl */

void title_hndl(void *data, const char *txt, int txtlen) {
  if(context == TITLE) {
    if(txtlen > 31)
      strncpy((char *)books[bookcount].title,txt,32);
    else 			
      strncpy((char *)books[bookcount].title,txt,txtlen);
  }
}

void char_hndl(void *data, const char *txt, int txtlen) {
  /** flow text on the fly, into page data structure.
      TODO handle UTF-8, don't simply skip non-ASCII chars! **/
  if(context != BODY) return;
	
  u16 i=0;
  u8 advance=0;
  static bool linebegan;
  page_t *page = &(pages[pagecurrent]);
	
  /** starting a new page? **/
  if(page->length == 0) {
    linebegan = false;
    ppen.x = MARGINLEFT;
    ppen.y = MARGINTOP + tsGetHeight();  }
  
  while(i<txtlen) {
    if(txt[i] == '\r') {
      i++;
      
    } else if(txt[i] > 0xc2 && txt[i] < 0xe0) {
      pagebuf[page->length] = '_';
      page->length++;
      i+=2;
      
    } else if(txt[i] > 0xdf && txt[i] < 0xf0) {
      pagebuf[page->length] = '_';
      page->length++;
      i+=3;
      
    } else if(txt[i] > 0xef) {
      pagebuf[page->length] = '_';
      page->length++;
      i+=4;
      
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
      for(j=i;(j<txtlen) && (!iswhitespace(txt[j]));j++) {
	/** set type until the end of the next word. **/
	advance += tsAdvance(txt[j]);
      }	
      int overrun = (ppen.x + advance) - (PAGE_WIDTH-MARGINRIGHT);
			
      if(overrun > 0) {
	/** we went over the margin. insert a break. **/
	pagebuf[page->length] = '\n';
	page->length++;
	ppen.x = MARGINLEFT;
	ppen.y += (tsGetHeight() + LINESPACING);
	if(ppen.y > (PAGE_HEIGHT-MARGINBOTTOM)) {
	  /** we are at the end of one of the facing pages. **/
	  if(fb == screen1) {
	    if(!page->buf) {
	      page->buf = malloc(page->length * sizeof(char));
	      if(!page->buf) tsString((u8*)"[alloc error]\n");
	    }
	    strncpy((char*)page->buf,(char*)pagebuf,page->length);
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
			
      /** append this word to the page. **/
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
      if(!page->buf) tsString((u8*)"[alloc error]\n");
    }
    strncpy((char*)page->buf,(char*)pagebuf,page->length);
    context = HTML;
  }
  if(!stricmp(el,"title")) context = HEAD;
  if(!stricmp(el,"head")) context = HTML;
}  /* End of end_hndl */

void proc_hndl(void *data, const char *target, const char *pidata) {
}  /* End proc_hndl */

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

void writebookpositions(void) {
  FILE *savefile = fopen("dslibris.ini","w");
  u8 i;
  for(i=0;i<bookcount;i++) {
    if(i == bookcurrent)
      fprintf(savefile,"%s %d\n",books[i].filename,pagecurrent);
    else
      fprintf(savefile,"%s 0\n",books[i].filename);
  }
  fclose(savefile);
}

void readbookposition(void) {
  FILE *savefile = fopen("dslibris.ini","r");
  if(savefile) {
    int pagesave;
    char filename[64];
    while(!feof(savefile)) {      
      fscanf(savefile, "%s %d\n", filename, &pagesave);
      if(!stricmp(filename,(char*)(books[bookcurrent].filename))) {
	pagecurrent = (u16)pagesave;
      }
    }
    fclose(savefile);
  }
}

void searchfortitle(u8 *filename) {
  char path[64];
  sprintf(path,"data/%s",filename);
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
  sprintf((char *)msg,"expat: [%s]\n",XML_ErrorString(XML_GetErrorCode(p)));
  tsString(msg);
  sprintf((char *)msg,"expat: [%d:%d] : %d\n",
	  (int)XML_GetCurrentLineNumber(p),
	  (int)XML_GetCurrentColumnNumber(p),
	  (int)XML_GetCurrentByteIndex(p));
  tsString(msg);
}

void hardwirebooks(void) {
  /** test function to debug book searching. **/
  bookcount = 3;
  bookcurrent = 1;
  initbook(&books[0]);
  strcpy((char*)books[0].filename,"jefferson.xhtml");
  strncpy((char*)books[0].title,"TO THE CITIZENS OF THE SOUTHERN STATES",16);

  initbook(&(books[1]));
  strcpy((char*)books[1].filename,"rhetorica.xhtml");
  strncpy((char*)books[1].title,"Rhetorica - Aristotle",16);
  initbook(&(books[2]));
  strcpy((char*)books[2].filename,"19211-h.xhtml");
  strncpy((char*)books[2].title,"History of England Vol I",16);
}

void drawpages(page_t *page) {
  drawblankpages();
  fb = screen0;
  tsInitPen();
  u16 i;
  for(i=0;i<page->length;i++) {
    u16 c = page->buf[i];
    if(c == '\n') {
      tsStartNewLine();
    } else {
      tsChar(c);
    }
  }
  //  writebookpositions();
}

int main(void) {
  bool browseractive = false;
	
  powerSET(POWER_LCD|POWER_2D_A|POWER_2D_B);
  irqInit();
#if 0
  /** wifi debugging **/
  {
    struct tcp_debug_comms_init_data init_data = {
      .port = 30000
    };
    if ( !init_debug( &tcpCommsIf_debug, &init_data)) {
      iprintf("Failed to initialise the debugger stub - cannot continue\n");
      drawsolid(31,0,0);
      while(1);
    }
  }

  //  debugHalt();
#endif
  irqEnable(IRQ_VBLANK);

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

  fatInitDefault();
  tsInitDefault();
  tsString((u8*)"[welcome to dslibris]\n");
  swiWaitForVBlank();

  bookcount = 0;
  bookcurrent = 0;
 
  /** find books, aka .xht files in data/. **/
  
  DIR_ITER *dp = diropen("data");
  if(!dp) {
    tsString((u8*)"[fatal: did not find data folder]\n");
    swiWaitForVBlank();
    while(true);
  }
  u8 filename[32];
  while(!dirnext(dp, (char*)filename, NULL)) {
    if((bookcount < MAXBOOKS)
       && strlen((char*)filename) > 4
       && (!stricmp(".xht",
		    (char*)(filename + (strlen((char *)filename)-4))))
       ) {
      initbook(&books[bookcount]);			
      strncpy((char*)books[bookcount].filename,(char*)filename,32);
      //      searchfortitle(filename);
      bookcount++;
    }
  }
  dirclose(dp);

  sprintf((char*)msg,"[found %d books]\n",bookcount);
  tsString(msg);
  swiWaitForVBlank();

  browseractive = true;
  makebrowser();
  drawbrowser();
  fb = screen0;
  swiWaitForVBlank();

  //  while(true);

  XML_Parser p = XML_ParserCreate(NULL);

  while(true) {
    scanKeys();
 
    if(browseractive) {
		
      if(keysDown() & KEY_A) {
	/** parse the selected book. **/

	swiWaitForVBlank();
	drawblankpages();
	swiWaitForVBlank();

	char path[64];		
	sprintf(path,"data/%s",books[bookcurrent].filename);
	FILE *fp = fopen(path,"r");
	if(!fp) {
	  sprintf((char *)msg,"[cannot open %s]\n",path);
	  tsString(msg);
	} else {
	  pagecount = 0;
	  pagecurrent = 0;
	  page_t *page = &(pages[pagecurrent]);
	  pageinit(page);

	  XML_ParserReset(p,NULL);
	  XML_UseParserAsHandlerArg(p);
	  XML_SetElementHandler(p, start_hndl, end_hndl);
	  XML_SetCharacterDataHandler(p, char_hndl);
	  XML_SetProcessingInstructionHandler(p, proc_hndl);

	  char filebuf[BUFSIZE];
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

	  drawblankpages();
	  fb = screen0;
	  tsInitPen();

	  pagecurrent = 0;
	  //	  readbookposition();
	  page = &(pages[pagecurrent]);
	  drawpages(page);
	  browseractive = false;
	}
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
			
      if(keysDown() & KEY_Y) {
	browseractive = true;
	drawbrowser();
	fb = screen0;
      }
			
      if(keysDown() & KEY_X) {
	drawblankpages();
      }

      if(keysDown() & KEY_START) {
	break;
      }
    }
    swiWaitForVBlank();
  }

  swiSoftReset();
  exit(0);
}

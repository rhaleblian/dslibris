#include <nds.h>		/* libnds */
#include <fat.h>   		/* maps stdio to FAT on ARM */
#include <expat.h>		/* expat - XML parsing */
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <errno.h>
#include "main.h"
#include "font.h"
#include "ui.h"

#define BUFSIZE 1024*64
#define PAGEBUFSIZE 2048
#define MAXPAGES 1024
#define MAXBOOKS 10

extern FT_Vector pen;
extern FT_GlyphSlotRec	glyphs[];
u16 *screen0, *screen1, *fb;

typedef struct book_s {
	char filename[32];
	char title[32];
//	char author[64];
//	int position;
} book_t;
book_t books[MAXBOOKS];
int bookcount;
int bookcurrent;

typedef struct page_s {
	char buf[PAGEBUFSIZE];
	int chars;
	int ibuf;		// index into buffer for parsing,
					// currently used to track last legal linebreak
} page_t;
page_t pages[MAXPAGES];
int pagecount;
int	pagecurrent;

typedef enum {NONE,HTML,HEAD,TITLE,BODY} context_t;
context_t context;

button_t buttons[16];

// struct initializers

void initbook(book_t *book) {
	strcpy(book->filename,"");
	strcpy(books->title,"");
//	strcpy(books[index].author,"");
//	books[index].position = 0;
}

void initpage(page_t *page) {
	strcpy(page->buf,"");
	page->chars = 0;
}

void drawsolid(int r, int g, int b) {
	u16 color = RGB15(r,g,b) | BIT(15);
	int i;
	for(i=0;i<PAGE_HEIGHT*PAGE_HEIGHT;i++) fb[i] =color;
}

void drawmargins() {
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

void drawblankpages() {
	fb = screen1;
	drawsolid(31,31,31);
	fb = screen0;
	drawsolid(31,31,31);
	tsInitPen();
}

int getjustifyspacing(page_t *page, int i) {
	// full justification.
	// get line advance, count spaces,
	// and insert more space in spaces.
	int spaces = 0;
	int advance = 0;
	int horispace = 0;
	int j,k;
	for(j=i;j<page->chars && page->buf[j]==' ';j++);
	for(j=i;j<page->chars && page->buf[j]!='\n';j++) {
		int c = (int)page->buf[j];
		advance += glyphs[c].advance.x >> 6;
		if(page->buf[j] == ' ') spaces++;
	}
	for(k=j;k>0 && page->buf[k]==' ';k--) spaces--;
	if(spaces) horispace = (float)((PAGE_WIDTH-MARGINRIGHT-MARGINLEFT) - advance) / (float)spaces;
	return(horispace);
}

void drawpages(page_t *page) {
	drawblankpages();
	tsInitPen();
	int i;
	for(i=0;i<page->chars;i++) {
		int c = (int)(page->buf[i]);
		if(c == '\n') {
			tsStartNewLine();
		} else {
			tsChar(c);
		}
	}
}

void drawbrowser() {
	fb = screen1;
	drawsolid(31,31,31);
	int i;
	for(i=0;i<bookcount;i++) {
		if(i==bookcurrent) drawbutton(&buttons[i],fb,1);
		else drawbutton(&buttons[i],fb,0);
	}
}


int iswhitespace(int c) {	if(c == ' ' || c == '\t' || c == '\n' || c == '\r') return 1;
	else return 0;
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
		if((strlen(books[bookcount].title) + txtlen) < 32)
			strcat(books[bookcount].title,txt);
		else
			strncpy(books[bookcount].title,txt,32);
	}
}

void char_hndl(void *data, const char *txt, int txtlen) {
	// paginate and linebreak on the fly into page data structure.
	// TODO txt is UTF-8, but for now we assume it's always in the ASCII range.
	if(context != BODY) return;
	
	int i=0;
	int advance=0;
	static bool linebegan;
	page_t *page = &pages[pagecurrent];
	
	if(page->chars == 0) linebegan = false;
	
	while(i<txtlen) {
		if(txt[i] == '\r') {
			i++;
		
		} else if(txt[i] > 127) {
			i++;
			
		} else if(iswhitespace((int)txt[i])) {
			if(linebegan) {
				page->buf[page->chars] = ' ';
				pen.x += (glyphs[' '].advance.x >> 6);
				page->chars++;
			}
			i++;

		} else {
			linebegan = true;
			int j;
			advance = 0;
			for(j=i;(j<txtlen) && (!iswhitespace((int)txt[j]));j++) {
				// set type until the end of the next word.
				advance += (glyphs[(int)txt[j]].advance.x >> 6);
			}			
			int overrun = (pen.x + advance) - (PAGE_WIDTH-MARGINRIGHT);
			
			if(overrun > 0) {
				// we went over the margin. insert a break.
				page->buf[page->chars] = '\n';
				page->chars++;
				pen.x = MARGINLEFT;
				pen.y += (tsGetHeight() + LINESPACING);
				if(pen.y > (PAGE_HEIGHT-MARGINBOTTOM)) {
					// we are at the end of one of the facing pages.
					if(fb == screen1) {
						pagecount++;
						pagecurrent++;
						page++;
						initpage(page);
						fb = screen0;
					} else {
						fb = screen1;
					}
					tsInitPen();
				}
				linebegan = false;
			}
			
			// append this word to the page.
			for(;i<j;i++) {
				if(iswhitespace(txt[i])) {
					if(linebegan) {
						page->buf[page->chars] = ' ';
						page->chars++;
					}
				} else {
					linebegan = true;
					page->buf[page->chars] = txt[i];
					page->chars++;
				}
			}
			pen.x += advance;
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
		page->buf[page->chars] = '\n';
		page->chars++;
		pen.x = MARGINLEFT;
		pen.y += tsGetHeight() + LINESPACING;
		if(pen.y > (PAGE_HEIGHT-MARGINBOTTOM)) {
			if(fb == screen1) {
				fb = screen0;
				pagecount++;
				pagecurrent++;
				page++;
				initpage(page);
			} else {
				fb = screen1;
			}
			tsInitPen();
		}
	}
	if(!stricmp(el,"html")) context = NONE;
	if(!stricmp(el,"body")) context = HTML;
	if(!stricmp(el,"title")) context = HEAD;
	if(!stricmp(el,"head")) context = HTML;
}  /* End of end_hndl */

void proc_hndl(void *data, const char *target, const char *pidata) {
}  /* End proc_hndl */

int bookparse(XML_Parser p) {
	char msg[64];
	char path[64];

	XML_UseParserAsHandlerArg(p);
	XML_SetElementHandler(p, start_hndl, end_hndl);
	XML_SetCharacterDataHandler(p, char_hndl);
	XML_SetProcessingInstructionHandler(p, proc_hndl);

	sprintf(path,"/data/%s",books[bookcurrent].filename);
	struct stat st;
	st.st_size = BUFSIZE;
	stat(path, &st);

	FILE *fp = fopen(path,"r");
	if(!fp) {
		sprintf(msg,"[cannot open %s]\n",path);
		tsString(msg);
		return(-1);
	}
	int bufsize;
	if(st.st_size < BUFSIZE) bufsize = (int)st.st_size;
	else bufsize = BUFSIZE;
	
	pagecount = 0;
	pagecurrent = 0;
	initpage(&pages[pagecurrent]);

	XML_Char *filebuf;
	filebuf = (char*)XML_GetBuffer(p, bufsize);				
	if(!filebuf) {
		sprintf(msg,"[allocating %d bytes failed]\n",bufsize);
		tsString(msg);
		return(-1);
	}
	
	while(1) {		
		sprintf(msg,"[%d bytes allocated]\n",bufsize);
		tsString(msg);

		int bytes_read = fread(filebuf, 1, bufsize, fp);
		sprintf(msg,"[%d bytes read]\n",bytes_read);
		tsString(msg);
		
		// parse and paginate.
		XML_ParseBuffer(p, bytes_read, bytes_read == 0);
		
		if(!bytes_read) break;
	}
	
	fclose(fp);

	pagecurrent = 0;
	tsInitPen();
	return(0);
}

void readbookpositions() {
	FILE *savefile = fopen("dslibris.ini","r");
	if(savefile) {
		int position = 0;
		int i;
		for(i=0;i<bookcount;i++) {
			if(fscanf(savefile,"%d\n", &position)) {
			//	books[i].position = position;
			}
		}
		fclose(savefile);
	}
}

int main(void) {
	bool browseractive = false;
	char msg[128];
	
	powerON(POWER_ALL);
	irqInit();
	irqEnable(IRQ_VBLANK);
	screen0 = (u16*)BG_BMP_RAM(0);
	screen1 = (u16*)BG_BMP_RAM_SUB(0);
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
	fb=screen0;
	swiWaitForVBlank();
	drawsolid(7,7,7);
	swiWaitForVBlank();
	fatInitDefault();
	drawsolid(15,15,15);
	swiWaitForVBlank();
	tsInitDefault();
	drawsolid(31,31,31);
	swiWaitForVBlank();
	tsString("[welcome to dslibris]\n[font loaded]\n");
	swiWaitForVBlank();

	// find all available books in the root directory.
	// files must be well-formed XHTML;
	// UVA HTML texts, for instance, need to go through HTML tidy.	
	bookcount = 0;
	bookcurrent = 0;
	sprintf(msg,"[searching for books]\n");
	tsString(msg);
	DIR_ITER *dp = diropen("/data");
	if(!dp) tsString("[diropen failed]\n");

	char filename[64];
	while(!dirnext(dp, filename, NULL)) {
		if((bookcount < MAXBOOKS)
			&& (!stricmp(".xhtml",filename + (strlen(filename)-6)))) {
			initbook(&books[bookcount]);
			strcpy(books[bookcount].filename,filename);
			FILE *fp = fopen(filename,"r");
			if(fp) {
				XML_Parser p = XML_ParserCreate(NULL);
				XML_UseParserAsHandlerArg(p);
				XML_SetElementHandler(p, start_hndl, end_hndl);
				XML_SetCharacterDataHandler(p, title_hndl);
				XML_SetProcessingInstructionHandler(p, proc_hndl);
				XML_Char *filebuf = (char*)XML_GetBuffer(p, BUFSIZE);
				while(1) {
					int bytes_read = fread(filebuf, 1, BUFSIZE, fp);
					XML_ParseBuffer(p, bytes_read, bytes_read == 0);
					if(!bytes_read) break;
				}
				XML_ParserFree(p);
				fclose(fp);
			}
			initbutton(&buttons[bookcount]);
			movebutton(&buttons[bookcount],0,bookcount*32);
			if(strlen(books[bookcount].title))
				strcpy(buttons[bookcount].text,books[bookcount].title);
			else
				strcpy(buttons[bookcount].text,books[bookcount].filename);
			bookcount++;
		}
	}
	dirclose(dp);
	sprintf(msg,"[%d books]\n",bookcount);
	tsString(msg);
	
	// restore reading positions.
	//readbookpositions();
	
	browseractive = true;
	swiWaitForVBlank();
	drawbrowser();
	
	while(1) {
		scanKeys();
 
		if(browseractive) {
			if(keysDown() & KEY_A) {
				fb = screen0;
				drawblankpages();
				tsInitPen();
				sprintf(msg,"[%s]\n",books[bookcurrent].filename);
				tsString(msg);

				pagecurrent = 0;
				pagecount = 0;
				initpage(&pages[pagecurrent]);

				XML_Parser p = XML_ParserCreate(NULL);
				if(bookparse(p)) {
					sprintf(msg,"[%s]\n",XML_ErrorString(XML_GetErrorCode(p)));
					tsString(msg);
					sprintf(msg,"expat: [%d:%d] : %d\n",
						(int)XML_GetCurrentLineNumber(p),
						(int)XML_GetCurrentColumnNumber(p),
						(int)XML_GetCurrentByteIndex(p));
					tsString(msg);
					drawbrowser();
				} else {
					tsInitPen();
					drawblankpages();
					pagecurrent = 0;
					drawpages(&pages[pagecurrent]);
					browseractive = false;
				}
				XML_ParserFree(p);
			}
			if(keysDown() & KEY_B) {
				browseractive = false;
				drawpages(&pages[pagecurrent]);
			}
			if(keysDown() & (KEY_LEFT|KEY_L)) {
				if(bookcurrent < bookcount-1) {
					bookcurrent++;
					drawbrowser();
				}
			}
			if(keysDown() & (KEY_RIGHT|KEY_R)) {
				if(bookcurrent > 0) {
					bookcurrent--;
					drawbrowser();
				}
			}
		} else {
			if(keysDown() & (KEY_A|KEY_DOWN|KEY_R)) {
				if(pagecurrent < pagecount-1) {
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
				drawbrowser(bookcurrent);
			}
			if(keysDown() & KEY_X) {
				drawblankpages();
			}
		}
		swiWaitForVBlank();
	}
}

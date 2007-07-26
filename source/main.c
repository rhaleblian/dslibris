#include <nds.h>		/* libnds */
#include <fat.h>   		/* maps stdio to FAT on ARM */
#include <expat.h>		/* expat - XML parsing */
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <errno.h>
#include "font.h"
#include "ui.h"

#define BUFSIZE 1024*512
#define PAGEBUFSIZE 2048
#define MAXPAGES 1024
#define MAXGLYPHS 256
#define MAXBOOKS 4
#define MARGINLEFT 10
#define MARGINRIGHT 10
#define MARGINTOP 10
#define MARGINBOTTOM 14
#define LINESPACINGADD 2
#define PIXELSIZE 12
#define DPI 72		    // probably not true for a DS - measure it
#define EMTOPIXEL (float)(POINTSIZE * DPI/72.0)
#define PAGE_HEIGHT SCREEN_WIDTH
#define PAGE_WIDTH SCREEN_HEIGHT

FT_Vector pen;
u16 *screen0, *screen1, *fb;
u16 *bb, bb0[256*256], bb1[256*256];

extern FT_Library library;
extern FT_Error   error;
extern FT_Face    face;
extern FT_GlyphSlotRec glyphs[];

typedef struct {
	char filename[32];
	char title[32];
} book_t;
book_t books[MAXBOOKS];
int bookcount;

typedef struct {
	char buf[PAGEBUFSIZE];
	int chars;
	int ibuf;		// index into buffer for parsing
} page_t;
page_t pages[MAXPAGES]; 
int pagecount;
int linebreak;

XML_Parser p;
typedef enum {HTML,HEAD,BODY} context_t;
context_t context;
int	curpage;
int pagedone;

button_t buttons[16];

// struct initializers

void initpage(int page) {
	strcpy(pages[page].buf,"");
	pages[page].chars = 0;
	pages[page].ibuf = 0;
}

FT_GlyphSlot initglyph(int i, FT_GlyphSlot src) {
	FT_GlyphSlot dst = &glyphs[i];
	int x = src->bitmap.rows;
	int y = src->bitmap.width;
	dst->bitmap.buffer = malloc(x*y);
	memcpy(dst->bitmap.buffer, src->bitmap.buffer, x*y);
	dst->bitmap.rows = src->bitmap.rows;
	dst->bitmap.width = src->bitmap.width;
	dst->bitmap_top = src->bitmap_top;
	dst->bitmap_left = src->bitmap_left;	
	dst->advance = src->advance;
	return(dst);
}

// unused and probably borken
void blitfacingpages() {
	memcpy(screen0,bb0,256*256*sizeof(u16));
	memcpy(screen1,bb1,256*256*sizeof(u16));
	swiWaitForVBlank();
}

void drawsolid(int r, int g, int b) {
	u16 color = RGB15(r,g,b) | BIT(15);
	int i;
	// memset here?
	for(i=0;i<PAGE_HEIGHT*PAGE_HEIGHT;i++) fb[i] =color;
}

void drawnewline() {
	pen.x = MARGINLEFT;
	pen.y += face->size->metrics.height >> 6;
	pen.y += LINESPACINGADD;
	if(pen.y > (PAGE_HEIGHT - MARGINBOTTOM)) {
		pen.y = MARGINTOP + (face->size->metrics.ascender >> 6);
		if(fb == screen0) {
			fb = screen1;
		} else {
			pagedone = 1;
		}
	}
}
 
void startpage() {
	fb = screen0;
	pen.x = MARGINLEFT;
	pen.y = MARGINTOP + (face->size->metrics.ascender >> 6);
	pagedone = 0;
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

void clearfacingpages() {
	fb = screen0;
	drawsolid(31,31,31);
//	drawmargins();
	fb = screen1;
	drawsolid(31,31,31);
//	drawmargins();
	fb = screen0;
}

void drawfacingpages(int pageindex) {
	startpage();
	clearfacingpages();
	swiWaitForVBlank();
	page_t *page = &(pages[pageindex]);

	int advance;
	int spaces;
	int i=0;
	while(i<page->chars && !pagedone) {
		advance = 0;
		spaces = 0;
		
		// full justification.
		// get line advance, count spaces,
		// and insert more space in spaces.
/*
		int j,k;
		for(j=i;j<page->chars && page->buf[j]==' ';j++);
		for(j=i;j<page->chars && page->buf[j]!='\n';j++) {
			int c = (int)page->buf[j];
			advance += glyphs[c].advance.x >> 6;
			if(page->buf[j] == ' ') spaces++;
		}
		for(k=j;k>0 && page->buf[k]==' ';k--) spaces--;
*/
		float horispace = 0.0;
		if(spaces) horispace = (float)((PAGE_WIDTH-MARGINRIGHT-MARGINLEFT) - advance) / (float)spaces;
		
		for(;i<page->chars;i++) {
			int c = (int)page->buf[i];
			if(c == '\n') {
				drawnewline();
			} else {
				if(c == ' ') pen.x += (int)(horispace);
				drawchar(c,&pen);
			}
		}
	}
}

void drawbrowser(int bookcurrent) {
	fb = screen1;
	drawsolid(31,31,31);
	int i;
	for(i=0;i<bookcount;i++) {
		if(i==bookcurrent) drawbutton(&buttons[i],fb,1);
		else drawbutton(&buttons[i],fb,0);
	}
}


int iswhitespace(int c) {
	if(c == ' ' || c == '\t' || c == '\n' || c == '\r') return 1;
	else return 0;
}

void
default_hndl(void *data, const char *s, int len) {
}  /* End default_hndl */

void
start_hndl(void *data, const char *el, const char **attr) {
	if(!stricmp(el,"html")) context = HTML;
	if(!stricmp(el,"body")) context = BODY;
	if(!stricmp(el,"head")) context = HEAD;
}  /* End of start_hndl */

void
end_hndl(void *data, const char *el) {
	page_t *page = &pages[curpage];
	if(
		!stricmp(el,"h1")
		|| !stricmp(el,"h2")
		|| !stricmp(el,"h3")
		|| !stricmp(el,"h4")
		|| !stricmp(el,"hr")
		|| !stricmp(el,"p")
		|| !stricmp(el,"br")
	) {
		page->buf[page->chars++] = '\n';
		page->ibuf = 0;
		drawnewline();
		if(pagedone) {
			curpage++;
			initpage(curpage);
			startpage();
		}
	}
}  /* End of end_hndl */

void
char_hndl(void *data, const char *txt, int txtlen) {
	// paginate and linebreak on the fly into page data structure.
	// TODO txt is UTF-8, but for now we assume it's always in the ASCII range.
	int i=0;
	page_t *page = &pages[curpage];
	
	while(i<txtlen) {
		if((int)txt[i] == '\r') {
			i++;
		} else if(iswhitespace((int)txt[i])) {
			if(page->ibuf) {
				pen.x += glyphs[(int)txt[i]].advance.x >> 6;
				page->buf[page->chars++] = ' ';
			}
			i++;
		} else {
			page->ibuf = 1; // non-whitespace
			int advance = 0;
			int j;
			for(j=i;(j<txtlen) && (!iswhitespace((int)txt[j]));j++) {
				// we're looking for the end of the next word.
				advance += glyphs[(int)txt[j]].advance.x >> 6;
			}
			if((pen.x + advance) > (PAGE_WIDTH - MARGINRIGHT)) {
				// we went over the margin. go back and break;
				// and pass to the next page if we pagebreak.
				page->buf[page->chars++] = '\n';
				drawnewline();  // this is at the pen position.
				page->ibuf = 0;
				if(pagedone) {
					initpage(++curpage);
					page++;
					startpage();
				}
			}
			for(;i<j;i++) {
				if(iswhitespace(txt[i]) && page->ibuf)
					page->buf[page->chars++] = ' ';
				else {
					page->ibuf = 1;
					page->buf[page->chars++] = txt[i];
				}
			}
			pen.x += advance;
		}
	}
}  /* End char_hndl */

void
proc_hndl(void *data, const char *target, const char *pidata) {
}  /* End proc_hndl */


int main(void) {
	int browseractive = 0;
	
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
	clearfacingpages();
	swiWaitForVBlank();

	if(!fatInitDefault()) {
		drawsolid(15,15,15);
		return -1;
	}
	
	// font.
	
	error = FT_Init_FreeType(&library);
    if(error) {
		drawsolid(0,31,0);
        return error;
	}
	error = FT_New_Face(library, "/frutiger.ttf", 0, &face);
	if(error) {
		drawsolid(0,0,31);
        return error;
	}
	FT_Set_Pixel_Sizes(face, 0, PIXELSIZE);
	
	// cache glyphs. glyphs[] will contain all the bitmaps.
	// using FirstChar() and NextChar() would be more robust.
	// TODO also cache kerning and transformations.
	int i;
	for(i=0;i<MAXGLYPHS;i++) {
		FT_Load_Char(face, i, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL);
		initglyph(i, face->glyph);
	}

	// parser.

	bookcount = 0;
	int bookcurrent = 0;
	book_t *book = books;

	// find all .xhtml files.
	// aside: files must be well-formed XML or XHTML.
	// UVA HTML texts, for instance, need to go through HTML tidy.	
	char filename[256];
	DIR_ITER* dp = diropen("/");
	while(dirnext(dp, filename, NULL) != ENOENT && bookcount < MAXBOOKS) {
		if(!stricmp(".xhtml",filename + (strlen(filename)-6))) {
			strcpy(book->filename, filename);
			if(bookcount < 4) {
				initbutton(&buttons[bookcount]);
				movebutton(&buttons[bookcount],MARGINLEFT,MARGINTOP + bookcount*20);
				strcpy(buttons[bookcount].text,book->filename);
			}
			bookcount++;
			book++;
		}
	}
	dirclose(dp);
	
	drawbrowser(bookcurrent);
	browseractive = 1;
	
	while(1) {
		scanKeys();
 
		if(browseractive) {
			if(keysDown() & KEY_A) {
				browseractive = 0;

				if(p) XML_ParserFree(p);
				if(!(p = XML_ParserCreate(NULL))) {
					drawsolid(31,0,0); return -1;
				}
				XML_UseParserAsHandlerArg(p);
				XML_SetElementHandler(p, start_hndl, end_hndl);
				XML_SetCharacterDataHandler(p, char_hndl);
				XML_SetProcessingInstructionHandler(p, proc_hndl);
	
				char path[64];
				strcpy(path,"/");
				strcat(path,books[bookcurrent].filename);
				FILE *fp = fopen(path,"r");
				if(!fp) {
					drawsolid(31,31,0);
					char msg[64];
					strcat(msg,"dslibris: error opening file ");
					strcat(msg,path);
					pen.x = 10; pen.y = 128;
					drawstring(msg, &pen);
					return -2;
				}
				struct stat filestat;
				if(fstat((int)fp,&filestat) == -1) filestat.st_size = BUFSIZE;
				XML_Char *filebuf = (char*)XML_GetBuffer(p, filestat.st_size);
				int bytes_read = fread(filebuf, 1, filestat.st_size, fp);
				fclose(fp);
				
				// parse and paginate.
				
				pagecount = 0;
				curpage = 0;
				initpage(curpage);
				startpage();
				error = XML_ParseBuffer(p, bytes_read, bytes_read == 0);
				pagecount = curpage+1;
	
				//draw page 0

				curpage = 0;
				drawfacingpages(curpage);
				swiWaitForVBlank();
			}	
			if(keysDown() & KEY_B) {
				browseractive = 0;
				drawfacingpages(curpage);
			}
			if(keysDown() & (KEY_LEFT|KEY_L)) {
				if(bookcurrent < 3) {
					bookcurrent++;
					drawbrowser(bookcurrent);
				}
			}
			if(keysDown() & (KEY_RIGHT|KEY_R)) {
				if(bookcurrent > 0) {
					bookcurrent--;
					drawbrowser(bookcurrent);
				}
			}
		} else {
		
			if(keysDown() & (KEY_A|KEY_DOWN|KEY_R)) {
				curpage++;
				drawfacingpages(curpage);
			}
			
			if(keysDown() & (KEY_B|KEY_UP|KEY_L)) {
				if(curpage > 0) {
					curpage--;
					drawfacingpages(curpage);
				}
			}
			
			if(keysDown() & KEY_Y) {
				drawbrowser(bookcurrent);
				browseractive = 1;
			}
		}
		swiWaitForVBlank();
	}

 	return 0;
}

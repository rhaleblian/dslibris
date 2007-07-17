#include <nds.h>		/* libnds */
#include <fat.h>   		/* maps stdio to FAT on ARM */
#include <libfat.h>
#include <ft2build.h>	/* freetype2 - text rendering */
#include FT_FREETYPE_H
#include <expat.h>		/* expat - XML parsing */
#include <sys/stat.h>

#define BUFSIZE 32000
#define PAGEBUFSIZE 2048
#define MAXPAGES 1024
#define MAXGLYPHS 256
#define MAXBOOKS 4
#define MARGINLEFT 8
#define MARGINRIGHT 8
#define MARGINTOP 8
#define MARGINBOTTOM 8
#define PIXELSIZE 12
#define DPI 72		    // probably not true for a DS - measure it
#define EMTOPIXEL (float)(POINTSIZE * DPI/72.0)
#define PAGE_HEIGHT SCREEN_WIDTH
#define PAGE_WIDTH SCREEN_HEIGHT

FT_Vector pen;
u16 *screen0, *screen1, *fb;
//u16 *bb, bb1[256*256], bb2[256*256];

FT_Library library;
FT_Error   error;
FT_Face    face;
FT_GlyphSlotRec glyphs[128];

typedef struct {
	char filename[32];
	char title[32];
} book_t;
book_t books[MAXBOOKS];
int bookcount;

typedef struct {
	char buf[PAGEBUFSIZE];
	int chars;
} page_t;
page_t pages[MAXPAGES]; 
int pagecount;
int linebreak;

XML_Parser p;
typedef enum {HTML,HEAD,BODY} context_t;
context_t context;
int	curpage;
int pagedone;

void solid(int r, int g, int b) {
	u16 color = RGB15(r,g,b) | BIT(15);
	int i;
	for(i=0;i<PAGE_HEIGHT*PAGE_HEIGHT;i++) fb[i] =color;
}

/*
void blitfacingpages() {
	memcpy(fb,bb,256*256*sizeof(u16));
	swiWaitForVBlank();
}
*/

void drawmargins() {
	int x, y; 
	for(y=0;y<PAGE_HEIGHT;y++) {
		fb[y*PAGE_HEIGHT + MARGINLEFT] = RGB15(30,30,30) | BIT(15);
		fb[y*PAGE_HEIGHT + PAGE_WIDTH-MARGINRIGHT] = RGB15(30,30,30) | BIT(15);
	}
	for(x=0;x<PAGE_HEIGHT;x++) {
		fb[MARGINTOP*PAGE_HEIGHT + x] = RGB15(30,30,30) | BIT(15);
		fb[(PAGE_HEIGHT-MARGINBOTTOM)*PAGE_HEIGHT + x] = RGB15(30,30,30) | BIT(15);
	}
}

void clearfacingpages() {
	fb = screen0;
	solid(31,31,31);
	drawmargins();
	fb = screen1;
	solid(31,31,31);
	drawmargins();
	fb = screen0;
}

void newpage(int page) {
	strcpy(pages[page].buf,"");
	pages[page].chars = 0;
}

FT_GlyphSlot newglyph(int i, FT_GlyphSlot src) {
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
 
void startpage() {
	fb = screen0;
	pen.y = MARGINTOP + (face->size->metrics.ascender >> 6);
	pen.x = MARGINLEFT;
	pagedone = 0;
}

void newline() {
	pen.x = MARGINLEFT;
	pen.y += face->size->metrics.height >> 6;
	if(pen.y > (PAGE_HEIGHT - MARGINBOTTOM)) {
		pen.y = MARGINTOP + (face->size->metrics.ascender >> 6);
		if(fb == screen0) {
			fb = screen1;
		} else {
			pagedone = 1;
		}
	}
}

void drawchar(int code) {
	// draw a character (ASCII) at the pen position.
	FT_GlyphSlot glyph = &glyphs[code];
	FT_Bitmap bitmap = glyph->bitmap;
	int bx = glyph->bitmap_left;
	int by = glyph->bitmap_top;
	int gx, gy;
	for(gy=0; gy<bitmap.rows; gy++) {
		for(gx=0; gx<bitmap.width; gx++) {
			/* get antialiased value */
			int a = bitmap.buffer[gy*bitmap.width+gx];
			if(a) {
				u8 sx = (pen.x+gx+bx);
				u8 sy = (pen.y+gy-by);			
				int l = (255-a) >> 3;
				fb[sy*SCREEN_WIDTH+sx] = RGB15(l,l,l) | BIT(15);
			}
		}
	}
}

void drawstring(char *string) {
	// draw an ASCII string starting at the pen position.
	int c, i;
	for(i=0;i<strlen(string);i++) {
		c = (int)string[i];
		drawchar(c);
	}
}

void writepage(int pageindex) {
	startpage();
	clearfacingpages();
	swiWaitForVBlank();
	int i;
	page_t *page = &(pages[pageindex]);
	for(i=0;(i<page->chars) && !pagedone;i++) {
		int c = (int)page->buf[i];
		if(c == '\n') newline();
		else {
			drawchar(c);
			pen.x += glyphs[c].advance.x >> 6;
		}
	}
//	blitfacingpages();
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
		page->buf[page->chars] = '\n';
		page->chars++;
		newline();
		if(pagedone) {
			curpage++;
			newpage(curpage);
			startpage();
		}
	}
}  /* End of end_hndl */

void
char_hndl(void *data, const char *txt, int txtlen) {
	// paginate on the fly into page data structure.
	// TODO txt is UTF-8, but for now we assume it's always in the ASCII range.
	int i=0;
	page_t *page = &pages[curpage];
	
	/* 
	while(i<txtlen) {
		if((int)txt[i] > 127) {}
		else if((int)txt[i] < 33) {
			ibreak = page->chars;
			page->buf[page->chars] = ' ';
			pen.x += glyphs[(int)txt[i]].advance.x >> 6;
			page->chars++;
		} else {
			// we're looking for the end of the next word.
			int advance = glyphs[(int)txt[i]].advance.x >> 6;
			int j;
			for(j=i+1;(j<txtlen) && ((int)txt[j] > 32) && ((int)txt[j] < 128);j++) {
				advance += glyphs[(int)txt[j]].advance.x >> 6;
			}
			if((pen.x + advance) > (PAGE_WIDTH - MARGINRIGHT)) {
				// we went over the margin. go back and break;
				// and pass to the next page if we pagebreak.
				page->buf[ibreak] = '\n';
				newline();  // this is at the pen position.
				if(pagedone) {
					curpage++;
					newpage(curpage);
					page++;					
					startpage();
					page = &pages[curpage];
				}
			}
			for(;i<j;i++) {
				page->buf[page->chars] = txt[i];
				page->chars++;
			}
			pen.x += advance;
		}
		i++;
	}
	*/
	
	while(i<txtlen) {
		int c = (int)txt[i];
		if(c == '\r') { i++; continue; }
		if(c < 33) {
			linebreak = page->chars;
			c = ' ';
		}
		page->buf[page->chars] = c;
		pen.x += glyphs[c].advance.x >> 6;
		if(c > 32 && pen.x > 182) {
			// a find a previous space to linebreak on
			int j;
			for(j=page->chars;(int)(page->buf[j]) > 32;j--);
			linebreak = j;
			page->buf[linebreak] = '\n';
			
			// linebreak and reposition pen at the end of new text on next line.
			newline();
			if(pagedone) {
				curpage++;
				newpage(curpage);
				page++;
				startpage();
			}
			int k;
			for(k=linebreak;k<=i;k++) pen.x += glyphs[(int)(page->buf[k])].advance.x >> 6;
		}	
		page->chars++;
		i++;
	}

}  /* End char_hndl */

void
proc_hndl(void *data, const char *target, const char *pidata) {
}  /* End proc_hndl */

int main(void){
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
	
	// font.
	
	fatInitDefault();
	FAT_InitFiles();
	error = FT_Init_FreeType(&library);
    if(error) {
		solid(31,0,0);
        return error;
	}
	error = FT_New_Face(library, "/frutiger.ttf", 0, &face);
	if(error) {
		solid(31,0,0);
        return error;
	}
	FT_Set_Pixel_Sizes(face, 0, PIXELSIZE);
	
	// cache advances, for pagination pass.
	// using FirstChar() and NextChar() would be more robust.
	// TODO also cache kerning and transformations.
	int i;
	for(i=0;i<MAXGLYPHS;i++) {
		FT_Load_Char(face, i, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL);
		newglyph(i, face->glyph);
	}

	// parser.

	if(!(p = XML_ParserCreate(NULL))) {
		solid(31,0,0); return -1;
	}
	XML_UseParserAsHandlerArg(p);
	XML_SetElementHandler(p, start_hndl, end_hndl);
	XML_SetCharacterDataHandler(p, char_hndl);
	XML_SetProcessingInstructionHandler(p, proc_hndl);

	// file. must be well-formed XML or XHTML.
	// UVA HTML texts, for instance, need to go through HTML tidy.
	
	// open up the first XML we find.
	int bookcount = 0;
	book_t *book = books;
	char filename[256];
	FILE_TYPE filetype = FAT_FindFirstFileLFN(filename);
	while(filetype != FT_NONE) {
		if(!stricmp(".xml",&(filename[strlen(filename)-5]))) {
			strcpy(book->filename, filename);
			bookcount++;
			break;
		}
		filetype = FAT_FindNextFileLFN(filename);
	}

	strcpy(filename,"/ebook.xml");
	FILE *fp = fopen(filename,"r");
	if(!fp) {
		solid(0,31,31);
		return -2;
	}
	struct stat st;
	if(fstat((int)fp,&st) == -1) st.st_size = BUFSIZE;
	XML_Char *filebuf = (char*)XML_GetBuffer(p, st.st_size);
	int bytes_read = fread(filebuf, 1, st.st_size, fp);
	fclose(fp);

	// parse and paginate.
	
	curpage = 0;
	newpage(curpage);
	pagecount = 0;
	error = XML_ParseBuffer(p, bytes_read, bytes_read == 0);
	XML_ParserFree(p);
	pagecount = curpage+1;
	
	//draw page 0
	
	curpage = 0;
	writepage(curpage);
	swiWaitForVBlank();
	
	while(1) {
		scanKeys();
 
		if((keysDown() & KEY_A) || (keysDown() & KEY_R)) {
			curpage++;
			writepage(curpage);
		}
		
		if((keysDown() & KEY_B) || (keysDown() & KEY_L)) {
			if(curpage > 0) {
				curpage--;
				writepage(curpage);
			}
		}
		
		swiWaitForVBlank();
	}
	
 	return 0;
}





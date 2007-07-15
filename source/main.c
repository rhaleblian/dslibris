/* 	TODO
parse HTML and strip
precompute and store pagination
forward page
back page
*/

#include <nds.h>      /* libnds */
#include <libfat.h>   /* maps stdio to FAT on ARM */
#include <pa9.h>      /* programmer's arsenal - libnds macros */
#include <ft2build.h> /* freetype2 - text rendering */
#include FT_FREETYPE_H
#include <expat.h>    /* expat - XML parsing */
#include <sys/stat.h>
#include <unistd.h>
	   
#define BUFSIZE 32000
#define PAGEBUFSIZE 1024
#define MAXPAGES 1024
#define MARGINLEFT 10
#define MARGINRIGHT 10
#define MARGINTOP 10
#define MARGINBOTTOM 10
#define LINESPACING 12	// cheese! use metrics
#define LETTERSPACING 8	// cheese! use metrics
#define PIXELSIZE 10
#define DPI 72		    // probably not true for a DS - measure it
#define EMTOPIXEL (float)(POINTSIZE * DPI/72.0)
#define PAGE_HEIGHT SCREEN_WIDTH
#define PAGE_WIDTH SCREEN_HEIGHT

XML_Parser p;
FT_Vector pen;
u16 *screen0, *screen1, *fb;
int Eventcnt = 0;
FT_Library library;
FT_Error   error;
FT_Face    face;
int	curpage;
int advance[128];		// glyph advances for the current font
int pagedone;
int breakingspace;     // where was the last legal char to break a line?
int nonwhitespace;     // have we found any nonwhitespace chars on this line?

typedef struct {
	char buf[PAGEBUFSIZE];
	int chars;
} page_t;
page_t pages[MAXPAGES]; 

typedef enum {HTML,HEAD,BODY} context_t;
context_t context;

void solid(int r, int g, int b) {
	int i;
	// TODO a memcpy() here instead
	for(i=0;i<PAGE_HEIGHT*PAGE_HEIGHT;i++) fb[i] = RGB15(r,g,b) | BIT(15);
}

void clearscreens() {
	fb = screen0;
	solid(31,31,31);
	fb = screen1;
	solid(31,31,31);
	fb = screen0;
}

void newpage(int page) {
	strcpy(pages[page].buf,"");
	pages[page].chars = 0;
	fb = screen0;
}

void startpage() {
	fb = screen0;
	pen.y = MARGINTOP + (face->size->metrics.ascender >> 6);
	pen.x = MARGINLEFT;
	pagedone = 0;
	breakingspace = 0;
	nonwhitespace = 0;
}

void newline() {
	int linespacing = face->size->metrics.height >> 6;
	if(pen.y + linespacing > PAGE_HEIGHT - MARGINBOTTOM) {
		// next screen or end of page
		if(fb == screen0) {
			fb = screen1;
			pen.y = MARGINTOP + (face->size->metrics.ascender >> 6);
		} else {
			pagedone = 1;
		}
	} else pen.y += linespacing;
	pen.x = MARGINLEFT;
	nonwhitespace = 0;
	breakingspace = 0;
}

void drawchar(int code) {
	// TODO slow! cache this data instead.
	FT_Load_Char(face, code, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL);
	FT_GlyphSlot glyph = face->glyph;
	FT_Bitmap bitmap = glyph->bitmap;
	int bx = glyph->bitmap_left;
	int by = glyph->bitmap_top;
	int gx, gy;
	for(gy=0; gy<bitmap.rows; gy++) {
		for(gx=0; gx<bitmap.width; gx++) {
			/* get antialiased value */
			int a = bitmap.buffer[gy*bitmap.width+gx];
			int l = (0*a + (255-a));

			u8 sx = (pen.x+gx+bx);
			u8 sy = (pen.y+gy-by);			
			fb[sy*SCREEN_WIDTH+sx] = RGB15(l>>3,l>>3,l>>3) | BIT(15);
		}
	}
}

void writepage(int p) {
	startpage();
	clearscreens();
	int i;
	for(i=0;i<pages[p].chars;i++) {
		int c = (int)pages[p].buf[i];
		if(c == '\n') newline();
		else {
			drawchar((int)(pages[p].buf[i]));
			pen.x += advance[(int)(pages[curpage].buf[i])];
		}
		if(pagedone) return;
	}
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
		!stricmp(el,"title")
		|| !stricmp(el,"h1")
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
	// TODO txt is UTF-8, but for now we assume it's always in the ASCII range.
	
	if(context != BODY) return;
	
	page_t *page = &pages[curpage];
	int i=0;
	while(i<txtlen) {
		int c = (int)txt[i];
		if(c > 127) continue;  // UTF-8...
		if(c == '\r') continue;
		if(c == '\t') c = ' ';
		if(c == '\n') c = ' ';

		if(c == ' ') {
			breakingspace = page->chars;
			if(!nonwhitespace) { i++; continue; } // don't add leading space
		} else nonwhitespace = 1;
		
		page->buf[page->chars] = c;
		page->chars++;
		if((pen.x + advance[c]) > (132)) {
			page->buf[breakingspace] = '\n';
			newline();
			if(pagedone) {
				curpage++;
				newpage(curpage);
				startpage();
				page++;
			}
		} else 	pen.x += advance[c];

		i++;
	}
}  /* End char_hndl */

void
proc_hndl(void *data, const char *target, const char *pidata) {
}  /* End proc_hndl */

int main(void){		
	int pagecount = 0;
	
	fatInitDefault();
	powerON(POWER_ALL);
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);
	irqInit();
	irqEnable(IRQ_VBLANK);
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);

	screen0 = (u16*)BG_BMP_RAM(0);
	screen1 = (u16*)BG_BMP_RAM_SUB(0);
	clearscreens();
	
	//initialize the backgrounds
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

	// font.
	
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
	// TODO also cache bitmaps, kerning and transformations.
	int i;
	for(i=0;i<128;i++) {
		FT_Load_Char(face, i, 
		FT_LOAD_RENDER // tell FreeType2 to render internally
		| FT_LOAD_TARGET_NORMAL); // grayscale 0-255
		advance[i] = face->glyph->advance.x >> 6;
	}

	// parser.

	p = XML_ParserCreate(NULL);
	if(!p) { solid(31,0,0); return -10; }
	XML_UseParserAsHandlerArg(p);
	XML_SetElementHandler(p, start_hndl, end_hndl);
	XML_SetCharacterDataHandler(p, char_hndl);
	XML_SetProcessingInstructionHandler(p, proc_hndl);

#ifdef DEBUG
	swiWaitForVBlank();
	solid(0,0,31);		
#endif

	// file. must be well-formed XML or XHTML.
	// UVA HTML texts, for instance, need to go through HTML tidy.
	
	FILE *fp = fopen("/rhetorica.xml","r");
	if(!fp) {
		solid(31,0,0);
		return -2;
	}
	struct stat st;
	if(fstat((int)fp,&st) == -1) st.st_size = BUFSIZE;
	XML_Char *filebuf = (char*)XML_GetBuffer(p, st.st_size);
	int bytes_read = fread(filebuf, 1, st.st_size, fp);
	fclose(fp);

#ifdef DEBUG
	fb = screen1;
	swiWaitForVBlank();
	solid(0,31,0);
	fb = screen0;
#endif

	// parse and paginate.
	
	curpage = 0;
	newpage(curpage);
	nonwhitespace = 0;
	error = XML_ParseBuffer(p, bytes_read, bytes_read == 0);
	XML_ParserFree(p);
	pagecount = curpage-1;
	
#ifdef DEBUG
	swiWaitForVBlank();
	solid(0,31,0);
#endif
	
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

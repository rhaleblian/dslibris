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
#include <expat.h>    /* expat - HTML parsing */
#include <sys/stat.h>
#include <unistd.h>
	   
#define BUFSIZE 32000
#define MARGINLEFT 10
#define MARGINRIGHT 10
#define MARGINTOP 8
#define MARGINBOTTOM 10
#define LINESPACING 12
#define LETTERSPACING 6
#define PIXELSIZE 14
#define PIXELSCALE 32
#define DPI 72
#define PAGE_HEIGHT SCREEN_HEIGHT
#define PAGE_WIDTH SCREEN_WIDTH

FT_Vector pen;
u16 *screen0, *screen1, *fb;
int Eventcnt = 0;
FT_Library library;
FT_Error   error;
FT_Face    face;
int	cursor, currentpage;
int pageindex[2048];	// page indices into the char buffer
int advance[128];		// glpyh advances for the current font
char buf[BUFSIZE];

int iswhitespace(char c) {
	if(c == ' ' || c == '\t' || c == '\n') return 1;
	return 0;
}

void paginate() {
	long i;
	int breakable = 0;
	int xpixels = MARGINLEFT;
	int line = 0;
	
	for(i=0;i<strlen(buf);i++) {
		if(iswhitespace(buf[i])) breakable = i;
		xpixels += advance[(int)buf[i]];
		if(xpixels > SCREEN_WIDTH - MARGINLEFT - MARGINRIGHT) {
			xpixels = 0;
			line++;
			if(line > 12) {
				pageindex[currentpage++] = i;
				line = 0;
			}
		}
	}
	pageindex[currentpage+1] = -1;  // last page
	currentpage = 0;
}

void solid(int r, int g, int b) {
	int i;
	for(i=0;i<SCREEN_HEIGHT*SCREEN_WIDTH;i++) fb[i] = RGB15(r,g,b) | BIT(15);
}

void startpage(FT_Face face) {
	pen.y = MARGINTOP + FT_MulFix(face->size->metrics.height,face->size->metrics.x_scale)/PIXELSCALE;
	pen.x = MARGINLEFT;
}

void clearpage() {
	fb = screen0;
	startpage(face);
	solid(31,31,31);
	fb = screen1;
	solid(31,31,31);
	fb = screen0;
}

void newline() {
	FT_Size_Metrics metrics = face->size->metrics;
	pen.x = MARGINLEFT;
	pen.y += FT_MulFix(face->size->metrics.height,metrics.y_scale)/PIXELSCALE;
}

int drawchar(int code) {
	/* x,y is in potrait page space
	sx, sy is in screen space, rotated */

	FT_Size_Metrics metrics = face->size->metrics;
	
	/* ignore returns */
	if(code == '\r') return 0;
	if(code == '\n') {
		pen.x = MARGINLEFT;
		pen.y += FT_MulFix(metrics.height, metrics.y_scale) / PIXELSCALE;
		return 0;
	}
	if(code == '\t') code = ' ';
	
	if(FT_Load_Char(face, code, 
		 FT_LOAD_RENDER // tell FreeType2 to render internally
		| FT_LOAD_TARGET_NORMAL // grayscale 0-255
		)) {
		solid(31,0,0);
		return -2;
	}
	FT_GlyphSlot glyph = face->glyph;
	FT_Bitmap bitmap = glyph->bitmap;
	
	/* space */
/*	if(code == '\n' || code == ' ' || code == '\t') {
		pen.x += FT_MulFix(glyph->advance.x, metrics.x_scale) / PIXELSCALE;
		return 0;
	}
*/
	/* wrap */
	if(pen.x + bitmap.width > PAGE_WIDTH - MARGINRIGHT) {
		pen.x = MARGINLEFT;
		pen.y += FT_MulFix(metrics.height, metrics.y_scale) / PIXELSCALE;
	}
	
	/* off page bottom */
	if(pen.y > PAGE_HEIGHT-MARGINBOTTOM) {
		if(fb == screen0) {
			fb = screen1;
			startpage(face);
		}
		else return -1;
	}

	int gx, gy;
	for(gy=0; gy<bitmap.rows; gy++) {
		for(gx=0; gx<bitmap.width; gx++) {
			/* get antialiased value */
			int a = bitmap.buffer[gy*bitmap.width+gx];
			int l = (0*a + (255-a));

			u8 sx = (pen.x+gx+glyph->bitmap_left);
			u8 sy = (pen.y+gy-glyph->bitmap_top);			
			fb[sy*SCREEN_WIDTH+sx] = RGB15(l>>3,l>>3,l>>3) | BIT(15);
		}
	}
	
	/* mark the origin */
//	fb[pen.y*PAGE_WIDTH+pen.x] = RGB15(31,0,0) | BIT(15);
	
	/* advance */
	pen.x += FT_MulFix(glyph->advance.x, metrics.x_scale)/(PIXELSCALE * .75);

	cursor++;
	return 0;
}

void printpage(int page) {    
	clearpage();
	int i;
	for(i=pageindex[page];i<pageindex[page+1];i++) {
//	for(i=0;i<10;i++) {
		drawchar(buf[i]);
	}
}

void
default_hndl(void *data, const char *s, int len) {
	int i;
	for(i=0;i<len;i++) {
		drawchar(s[i]);
	}
}  /* End default_hndl */

void
printcurrent(XML_Parser p) {
  XML_SetDefaultHandler(p, default_hndl);
  XML_DefaultCurrent(p);
  XML_SetDefaultHandler(p, (XML_DefaultHandler) 0);
}  /* End printcurrent */

void
start_hndl(void *data, const char *el, const char **attr) {
}  /* End of start_hndl */


void
end_hndl(void *data, const char *el) {
	if(!stricmp(el,"br")
		|| !stricmp(el,"title")
		|| !stricmp(el,"h1")
		|| !stricmp(el,"h2")
		|| !stricmp(el,"h3")
		|| !stricmp(el,"h4")
		|| !stricmp(el,"hr")) buf[cursor++] = '\n';
}  /* End of end_hndl */

void
char_hndl(void *data, const char *txt, int txtlen) {
	int i;
	for(i=0;i<txtlen;i++) buf[cursor++] = txt[i];
}  /* End char_hndl */

void
proc_hndl(void *data, const char *target, const char *pidata) {
}  /* End proc_hndl */

int main(void){		
	screen0 = (u16*)BG_BMP_RAM(0);
	screen1 = (u16*)BG_BMP_RAM_SUB(0);
	fb = screen0;
	
	fatInitDefault();
	powerON(POWER_ALL);
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);
	irqInit();
	irqEnable(IRQ_VBLANK);
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	
	//initialize the background
	BACKGROUND.control[3] = BG_BMP16_256x256 | BG_BMP_BASE(0);
	
	BACKGROUND.bg3_rotation.xdy = 0;
	BACKGROUND.bg3_rotation.xdx = 1 << 8;
	BACKGROUND.bg3_rotation.ydx = 0;
	BACKGROUND.bg3_rotation.ydy = 1 << 8;
 
	//initialize the sub background
	BACKGROUND_SUB.control[3] = BG_BMP16_256x256 | BG_BMP_BASE(0);
	
	BACKGROUND_SUB.bg3_rotation.xdy = 0;
	BACKGROUND_SUB.bg3_rotation.xdx = 1 << 8;
	BACKGROUND_SUB.bg3_rotation.ydx = 0;
	BACKGROUND_SUB.bg3_rotation.ydy = 1 << 8;

    error = FT_Init_FreeType(&library);
    if(error) {
		solid(31,31,0);
        return error;
	}
	error = FT_New_Face(library, "/frutiger.ttf", 0, &face);
	if(error) {
		solid(0,0,31);
        return error;
	}
	FT_Set_Pixel_Sizes(face, 0, PIXELSIZE);
	// cache advances for later pagination.
	int ascii;
	for(ascii=0;ascii<128;ascii++) {
		FT_Load_Char(face, ascii, 
		 FT_LOAD_RENDER // tell FreeType2 to render internally
		| FT_LOAD_TARGET_NORMAL); // grayscale 0-255
		advance[ascii] = FT_MulFix(face->glyph->advance.x, face->size->metrics.x_scale) / PIXELSCALE;
	}

	XML_Parser p = XML_ParserCreate(NULL);
	if(!p) { solid(31,0,0); return -10; }

	FILE *fp = fopen("/coe.htm","r");
	if(!fp) {
		solid(31,0,0);
		return -2;
	}
	
	struct stat s;
//	if(fstat(fp,&s) == -1) { solid(0,31,0); fclose(fp); return -1; }
	s.st_size = BUFSIZE;
	
	XML_Char *filebuf = (char*)XML_GetBuffer(p, s.st_size);
	int bytes_read = fread(filebuf, 1, s.st_size, fp);
	fclose(fp);
	
	XML_UseParserAsHandlerArg(p);
	XML_SetElementHandler(p, start_hndl, end_hndl);
	XML_SetCharacterDataHandler(p, char_hndl);
	XML_SetProcessingInstructionHandler(p, proc_hndl);

	cursor = 0;
	XML_ParseBuffer(p, bytes_read, bytes_read == 0);
	XML_ParserFree(p);
	buf[cursor] = 0;   // terminate string
	currentpage = 0;
	clearpage();
	paginate();
	currentpage = 0;
	printpage(currentpage);
	
	while(1) {
		scanKeys();
 
		if(keysDown() & KEY_A) {
			printpage(++currentpage);
		}
		
		if(keysDown() & KEY_B) {
			if(currentpage > 0) printpage(--currentpage);
		}
		
		swiWaitForVBlank();
	}
	
 	return 0;
}

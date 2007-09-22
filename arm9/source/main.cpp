/**----------------------------------------------------------------------------
   $Id: main.cpp,v 1.1 2007/09/21 22:18:05 rhaleblian Exp $
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
#include "types.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"
#include "main.h"
#include "parse.h"
#include "wifi.h"

#define APP_VERSION "0.2.4"
#define APP_URL "http://rhaleblian.wordpress.com"

u16 *screen0, *screen1, *fb;

Book books[MAXBOOKS];
u8 bookcount, bookcurrent;

page_t pages[MAXPAGES];
u8 pagebuf[PAGEBUFSIZE];
u16 pagecount, pagecurrent;

Text *ts;
Button *buttons[MAXBOOKS];

char msg[128];
bool invert = false;
parsedata_t parsedata;

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

void prefs_start_hndl(	void *data,
                       const XML_Char *name,
                       const XML_Char **attr);
bool prefs_read(XML_Parser p);
void prefs_write(void);

inline void spin(void)
{
	while (true) swiWaitForVBlank();
}

/*---------------------------------------------------------------------------*/

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
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	vramSetBankC(VRAM_C_SUB_BG);
	SUB_BG0_CR = BG_MAP_BASE(31);
	{
		u32 i;
		for (i=0;i<255;i++)
			BG_PALETTE_SUB[i] = RGB15(0,0,0);
		BG_PALETTE_SUB[255] = RGB15(15,31,15);
		consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31),
		                   (u16*)CHAR_BASE_BLOCK_SUB(0), 16);
	}
	printf(" startup console       [  OK  ]\n");
	swiWaitForVBlank();

	printf(" media filesystem      ");
	if (!fatInitDefault())
	{
		printf("[ FAIL ]\n");
		spin();
	}
	else printf("[  OK  ]\n");
	swiWaitForVBlank();

	printf(" typesetter            ");
	ts = new Text();
	if (ts->InitDefault())
	{
		printf("[ FAIL ]\n");
		spin();
	}
	else printf("[  OK  ]\n");
	swiWaitForVBlank();

	/** assemble library by indexing all
		XHTML/XML files in the current directory.
	    TODO recursive book search **/

	printf(" book scan             ");
	bookcount = 0;
	bookcurrent = 0;
	char filename[64];
	char dirname[16] = ".";
	DIR_ITER *dp = diropen(dirname);
	if (!dp)
	{
		printf("[ FAIL ]\n");
		spin();
	}
	while (!dirnext(dp, filename, NULL) && (bookcount != MAXBOOKS))
	{
		char *c;
		for (c=filename;c!=filename+strlen(filename) && *c!='.';c++);
		if (!stricmp(".xht",c) || !stricmp(".xhtml",c))
		{
			Book *book = &(books[bookcount]);
			book->SetFilename(filename);
			bookcount++;
		}
	}
	dirclose(dp);
	if (!bookcount)
	{
		printf("[ FAIL ]\n");
		spin();
	}
	else printf("[  OK  ]\n");
	swiWaitForVBlank();
	browser_init();

	printf(" expat XML parser      ");
	XML_Parser p = XML_ParserCreate(NULL);
	if (!p)
	{
		printf("[ FAIL ]\n");
		spin();
	}
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
	screen1 = (u16*)BG_BMP_RAM(0);

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

/*
	if (prefs_read(p))
	{
		page_clearinvert(screen0);
		page_clearinvert(screen1);
		ts->SetInvert(true);
		ts->InitPen();
		ts->PrintString("[loading...]");
		ts->SetInvert(false);
		swiWaitForVBlank();

		int p = pagecurrent;
		pagecount = 0;
		pagecurrent = 0;
		page_t *page = &(pages[pagecurrent]);
		page_init(page);
		books[bookcurrent].Parse(filebuf);
		pagecurrent = p;

		fb = screen0;
		ts->InitPen();
		page = &(pages[pagecurrent]);
		page_draw(page);
		browseractive = false;
	}
	else
*/
	{
		browser_init();
		splash_draw();
		browser_draw();
		browseractive = true;
	}
	swiWaitForVBlank();

	touchPosition touch;

	while (true)
	{
		scanKeys();

		if (browseractive)
		{
			if (keysDown() & KEY_TOUCH)
			{
				touch = touchReadXY();
				fb[touch.px + touch.py * 256] = rand();
				if (pagecurrent < pagecount)
				{
					pagecurrent++;
					page_draw(&pages[pagecurrent]);
				}
			}

			if (keysDown() & KEY_A)
			{
				/** parse the selected book. **/

				swiWaitForVBlank();
				page_clear();

				pagecount = 0;
				pagecurrent = 0;
				page_t *page = &(pages[pagecurrent]);
				page_init(page);

				page_clearinvert(screen0);
				page_clearinvert(screen1);
				ts->SetInvert(true);
				ts->SetScreen(screen0);
				ts->InitPen();
				ts->PrintString("[loading...]");
				ts->SetInvert(false);
				if (!books[bookcurrent].Parse(filebuf))
				{
					fb = screen0;
					ts->SetScreen(screen0);
					ts->InitPen();
					pagecurrent = 0;
					page = &(pages[pagecurrent]);
					page_draw(page);
					//prefs_write();
					browseractive = false;
				}
				else
				{
					browser_draw();
				}
			}

			if (keysDown() & KEY_B)
			{
				browseractive = false;
				page_draw(&pages[pagecurrent]);
			}

			if (keysDown() & (KEY_LEFT|KEY_L))
			{
				if (bookcurrent < bookcount-1)
				{
					bookcurrent++;
					browser_draw();
				}
			}

			if (keysDown() & (KEY_RIGHT|KEY_R))
			{
				if (bookcurrent > 0)
				{
					bookcurrent--;
					browser_draw();
				}
			}

			if (keysDown() & KEY_SELECT)
			{
				browseractive = false;
				page_draw(&(pages[pagecurrent]));
			}
		}
		else
		{
			if (keysDown() & (KEY_A|KEY_DOWN|KEY_R))
			{
				if (pagecurrent < pagecount)
				{
					pagecurrent++;
					page_draw(&pages[pagecurrent]);
					//prefs_write();
				}
			}

			if (keysDown() & (KEY_B|KEY_UP|KEY_L))
			{
				if (pagecurrent > 0)
				{
					pagecurrent--;
					page_draw(&pages[pagecurrent]);
					//prefs_write();
				}
			}

			if (keysDown() & KEY_SELECT)
			{
				//prefs_write();
				browseractive = true;
				splash_draw();
				browser_draw();
			}

			if(keysDown() & KEY_START)
			{
				break;
			}
		}
		swiWaitForVBlank();
	}

	XML_ParserFree(p);
	exit(0);
}

void browser_init(void)
{
	u8 i;
	for (i=0;i<bookcount;i++)
	{
		buttons[i] = new Button();
		buttons[i]->Init(ts);
		buttons[i]->Move(0,(i+1)*32);
		if (strlen(books[i].GetTitle()))
			buttons[i]->Label(books[i].GetTitle());
		else
			buttons[i]->Label(books[i].GetFilename());
	}	
}

void browser_draw(void)
{
	ts->SetScreen(screen1);
	page_drawsolid(0,0,0);
	int i;
	for (i=0;i<bookcount;i++)
	{
		if (i==bookcurrent)
			buttons[i]->Draw(screen1,true);
		else
			buttons[i]->Draw(screen1,false);
	}
}

void parse_printerror(XML_Parser p)
{
	sprintf(msg,"expat: [%s]\n",XML_ErrorString(XML_GetErrorCode(p)));
	ts->PrintString(msg);
	sprintf(msg,"expat: [%d:%d] : %d\n",
	        (int)XML_GetCurrentLineNumber(p),
	        (int)XML_GetCurrentColumnNumber(p),
	        (int)XML_GetCurrentByteIndex(p));
	ts->PrintString(msg);
}


void page_init(page_t *page)
{
	page->length = 0;
	page->buf = NULL;
}

u8 page_getjustifyspacing(page_t *page, u16 i)
{
	/** full justification. get line advance, count spaces,
	    and insert more space in spaces to reach margin.
	    returns amount of space to add per-character. **/

	u8 spaces = 0;
	u8 advance = 0;
	u8 j,k;

	/* walk through leading spaces */
	for (j=i;j<page->length && page->buf[j]==' ';j++);

	/* find the end of line */
	for (j=i;j<page->length && page->buf[j]!='\n';j++)
	{
		u16 c = page->buf[j];
		advance += ts->Advance(c);

		if (page->buf[j] == ' ') spaces++;
	}

	/* walk back through trailing spaces */
	for (k=j;k>0 && page->buf[k]==' ';k--) spaces--;

	if (spaces)
		return((u8)((float)((PAGE_WIDTH-MARGINRIGHT-MARGINLEFT) - advance)
		            / (float)spaces));
	else return(0);
}

void page_drawsolid(u8 r, u8 g, u8 b)
{
	u16 color = RGB15(r,g,b) | BIT(15);
	int i;
	for (i=0;i<PAGE_HEIGHT*PAGE_HEIGHT;i++) fb[i] =color;
}

void page_clear(void)
{
	u16 *savefb = fb;
	fb = screen0;
	page_drawsolid(31,31,31);
	fb = screen1;
	page_drawsolid(31,31,31);
	fb = savefb;
}

void page_clearone(u16 *screen)
{
	u16 *savefb = fb;
	fb = screen;
	page_drawsolid(31,31,31);
	fb = savefb;
}

void page_clearinvert(u16 *screen)
{
	u16 *savefb = fb;
	fb = screen;
	page_drawsolid(0,0,0);
	fb = savefb;
}

#define SPLASH_LEFT (MARGINLEFT+28)
#define SPLASH_TOP (MARGINTOP+96)

void splash_draw(void)
{
	ts->SetInvert(true);

	ts->SetScreen(screen0);
	ts->SetPixelSize(36);
	page_clearinvert(screen0);
	ts->SetPen(SPLASH_LEFT,SPLASH_TOP);
	ts->PrintString("dslibris");
	ts->SetPixelSize(0);
	ts->SetPen(SPLASH_LEFT,ts->GetPenY()+ts->GetHeight());
	ts->PrintString("an ebook reader");
	ts->SetPen(SPLASH_LEFT,ts->GetPenY()+ts->GetHeight());
	ts->PrintString("for Nintendo DS");
	ts->SetPen(SPLASH_LEFT,ts->GetPenY()+ts->GetHeight());
	ts->PrintString(APP_VERSION);
	ts->PrintNewLine();
	ts->SetPen(SPLASH_LEFT,ts->GetPenY()+ts->GetHeight());
	sprintf(msg,"%d books\n", bookcount);
	ts->PrintString(msg);

	ts->SetScreen(screen1);
	ts->SetPen(MARGINLEFT+100, MARGINTOP+16);
	ts->SetPixelSize(20);
	ts->PrintString("library");
	ts->SetPixelSize(0);

	ts->SetInvert(false);
}

void page_draw(page_t *page)
{
	page_clear();
	ts->SetScreen(screen0);
	ts->InitPen();
	u16 i=0;
	while (i<page->length)
	{
		u16 c = page->buf[i];
		if (c == '\n')
		{
			if(!ts->PrintNewLine()) break;
			i++;
		}
		else
		{
			if (c > 127) i+=ts->GetUCS((char*)&(page->buf[i]),&c);
			else i++;
			ts->PrintChar(c);
		}
	}

	ts->SetScreen(screen1);
	u8 offset = (int)(170.0 * (pagecurrent / (float)pagecount));
	ts->SetPen(MARGINLEFT+offset,250);
	sprintf((char*)msg,"[%d]",pagecurrent+1);
	ts->PrintString(msg);
}

int min(int x, int y)
{
	if (y < x) return y;
	else return x;
}

bool iswhitespace(u8 c)
{
	switch (c)
	{
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

void parse_init(parsedata_t *data)
{
	data->stacksize = 0;
	data->book = NULL;
	data->page = NULL;
	data->pen.x = MARGINLEFT;
	data->pen.y = MARGINTOP;
}

void parse_push(parsedata_t *data, context_t context)
{
	data->stack[data->stacksize++] = context;
}

context_t parse_pop(parsedata_t *data)
{
	if (data->stacksize) data->stacksize--;
	return data->stack[data->stacksize];
}

bool parse_in(parsedata_t *data, context_t context)
{
	u8 i;
	for (i=0;i<data->stacksize;i++)
	{
		if (data->stack[i] == context) return true;
	}
	return false;
}

bool parse_pagefeed(parsedata_t *data, page_t *page)
{
	int pagedone;

	/** we are at the end of one of the facing pages. **/
	if (fb == screen1)
	{

		/** we left the right page, save chars into this page. **/
		if (!page->buf)
		{
			page->buf = new u8[page->length];
			if (!page->buf)
			{
				ts->PrintString("[out of memory]\n");
				spin();
			}
		}
		memcpy(page->buf,pagebuf,page->length * sizeof(u8));
		fb = screen0;
		pagedone = true;

	}
	else
	{
		fb = screen1;
		pagedone = false;
	}
	data->pen.x = MARGINLEFT;
	data->pen.y = MARGINTOP + ts->GetHeight();
	return pagedone;
}

void prefs_start_hndl(	void *userdata,
						const XML_Char *name,
						const XML_Char **attr)
{
	if (!stricmp(name,"bookmark"))
	{
		Book *books = (Book*)userdata;
		char filename[64];
		s16 position = -1;
		u8 i;
		for (i=0;attr[i];i+=2)
		{
			if (!strcmp(attr[i],"file")) strcpy(filename, attr[i+1]);
			if (!strcmp(attr[i],"position")) position = atoi(attr[i+1]);
		}
		for(i=0; i<bookcount; i++)
		{
			if(!stricmp(books[i].GetFilename(),filename))
			{
				books[i].SetPosition(position);
				break;
			}			
		}
	}
}

/** side effect - sets bookcurrent to index of bookmarked book. **/

bool prefs_read(XML_Parser p)
{
	FILE *fp = fopen("dslibris.xml","r");
	if (!fp) return false;
	prefsdata_t data;
	strcpy(data.filename,"");
	data.position = 0;

	XML_ParserReset(p, NULL);
	XML_SetStartElementHandler(p, prefs_start_hndl);
	XML_SetUserData(p, (void *)books);
	while (true)
	{
		void *buff = XML_GetBuffer(p, 64);
		int bytes_read = fread(buff, sizeof(char), 64, fp);
		XML_ParseBuffer(p, bytes_read, bytes_read == 0);
		if (bytes_read == 0) break;
	}
	fclose(fp);
	return true;
}

void prefs_write(void)
{
	FILE* fp = fopen("dslibris.xml","w+");
	if(fp) {
		fprintf(fp,"<dslibris>\n");
		fprintf(fp, "<bookmark file=\"%s\" position=\"%d\" />\n",
		        books[bookcurrent].GetFilename(), pagecurrent);
		for(u8 i=0;i<bookcount; i++)
		{
			fprintf(fp, "<book file=\"%s\" />\n",books[i].GetFilename());
		}
		fprintf(fp, "\n</dslibris>\n");
		fclose(fp);
	}
}

int unknown_hndl(void *encodingHandlerData,
                 const XML_Char *name,
                 XML_Encoding *info)
{
	return(XML_STATUS_ERROR);
}

void default_hndl(void *data, const XML_Char *s, int len)
{
	parsedata_t *p = (parsedata_t *)data;
	if (s[0] == '&')
	{
		page_t *page = &(pages[pagecurrent]);

		/** handle only common iso-8859-1 character codes. */
		if (!strnicmp(s,"&nbsp;",5))
		{
			pagebuf[page->length++] = ' ';
			p->pen.x += ts->Advance(' ');
			return;
		}

		/** if it's numeric, convert to UTF-8. */
		int code=0;
		sscanf(s,"&#%d;",&code);
		if (code)
		{
			if (code>=128 && code<=2047)
			{
				pagebuf[page->length++] = 192 + (code/64);
				pagebuf[page->length++] = 128 + (code%64);
			}

			if (code>=2048 && code<=65535)
			{
				pagebuf[page->length++] = 224 + (code/4096);
				pagebuf[page->length++] = 128 + ((code/64)%64);
				pagebuf[page->length++] = 128 + (code%64);
			}

			p->pen.x += ts->Advance(code);
		}
	}
}  /* End default_hndl */

void start_hndl(void *data, const char *el, const char **attr)
{
	parsedata_t *pdata = (parsedata_t*)data;
	if (!stricmp(el,"html")) parse_push(pdata,HTML);
	if (!stricmp(el,"body")) parse_push(pdata,BODY);
	if (!stricmp(el,"title")) parse_push(pdata,TITLE);
	if (!stricmp(el,"head")) parse_push(pdata,HEAD);
	if (!stricmp(el,"pre")) parse_push(pdata,PRE);
}  /* End of start_hndl */

void title_hndl(void *userdata, const char *txt, int txtlen)
{
	parsedata_t *data = (parsedata_t*)userdata;
	if (parse_in(data,TITLE))
	{
		if (txtlen > 30)
		{
			char title[32];
			strncpy(title, txt, 27);
			strcat(title, "...");
			(data->book)->SetTitle(title);
		}
		else
		{
			(data->book)->SetTitle(txt);
		}
	}
}

void char_hndl(void *data, const XML_Char *txt, int txtlen)
{
	/** flow text on the fly, into page data structure. **/

	parsedata_t *pdata = (parsedata_t *)data;
	if (!parse_in(pdata,BODY)) return;

	int i=0;
	u8 advance=0;
	static bool linebegan=false;
	page_t *page = &(pages[pagecurrent]);

	/** starting a new page? **/
	if (page->length == 0)
	{
		linebegan = false;
		pdata->pen.x = MARGINLEFT;
		pdata->pen.y = MARGINTOP + ts->GetHeight();
	}

	while (i<txtlen)
	{
		if (txt[i] == '\r')
		{
			i++;
			continue;
		}

		if (iswhitespace(txt[i]))
		{
			if (parse_in(pdata,PRE) && txt[i] == '\n')
			{
				pagebuf[page->length++] = txt[i];
				pdata->pen.x += ts->Advance((u16)txt[i]);
				pdata->pen.y += (ts->GetHeight() + LINESPACING);
				if (pdata->pen.y > (PAGE_HEIGHT-MARGINBOTTOM))
				{
					if (parse_pagefeed(pdata,page))
					{
						page++;
						page_init(page);
						pagecurrent++;
						pagecount++;
					}
					linebegan = false;
				}

			}
			else
			{
				if (linebegan)
				{
					pagebuf[page->length++] = ' ';
					pdata->pen.x += ts->Advance((u16)' ');
				}

			}
			i++;

		}
		else
		{
			linebegan = true;
			int j;
			advance = 0;
			u8 bytes = 1;
			for (j=i;(j<txtlen) && (!iswhitespace(txt[j]));j+=bytes)
			{

				/** set type until the end of the next word.
				    account for UTF-8 characters when advancing. **/
				u16 code;
				if (txt[j] > 127)
					bytes = ts->GetUCS((char*)&(txt[j]),&code);
				else
				{
					code = txt[j];
					bytes = 1;
				}
				advance += ts->Advance(code);
			}

			/** reflow. if we overrun the margin, insert a break. **/

			int overrun = (pdata->pen.x + advance) - (PAGE_WIDTH-MARGINRIGHT);
			if (overrun > 0)
			{
				pagebuf[page->length] = '\n';
				page->length++;
				pdata->pen.x = MARGINLEFT;
				pdata->pen.y += (ts->GetHeight() + LINESPACING);

				if (pdata->pen.y > (PAGE_HEIGHT-MARGINBOTTOM))
				{
					if (parse_pagefeed(pdata,page))
					{
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
			for (;i<j;i++)
			{
				if (iswhitespace(txt[i]))
				{
					if (linebegan)
					{
						pagebuf[page->length] = ' ';
						page->length++;
					}
				}
				else
				{
					linebegan = true;
					pagebuf[page->length] = txt[i];
					page->length++;
				}
			}
			pdata->pen.x += advance;
		}
	}
}  /* End char_hndl */

void end_hndl(void *data, const char *el)
{
	page_t *page = &pages[pagecurrent];
	parsedata_t *p = (parsedata_t *)data;
	if (
	    !stricmp(el,"br")
	    || !stricmp(el,"p")
	    || !stricmp(el,"h1")
	    || !stricmp(el,"h2")
	    || !stricmp(el,"h3")
	    || !stricmp(el,"h4")
	    || !stricmp(el,"hr")
	)
	{
		pagebuf[page->length] = '\n';
		page->length++;
		p->pen.x = MARGINLEFT;
		p->pen.y += ts->GetHeight() + LINESPACING;
		if ( !stricmp(el,"p"))
		{
			pagebuf[page->length] = '\n';
			page->length++;
			p->pen.y += ts->GetHeight() + LINESPACING;
		}
		if (p->pen.y > (PAGE_HEIGHT-MARGINBOTTOM))
		{
			if (fb == screen1)
			{
				fb = screen0;
				if (!page->buf)
					page->buf = (u8*)new u8[page->length];
				strncpy((char*)page->buf,(char *)pagebuf,page->length);
				page++;
				page_init(page);
				pagecurrent++;
				pagecount++;
			}
			else
			{
				fb = screen1;
			}
			p->pen.x = MARGINLEFT;
			p->pen.y = MARGINTOP + ts->GetHeight();
		}
	}
	if (!stricmp(el,"body"))
	{
		if (!page->buf)
		{
			page->buf = new u8[page->length];
			if (!page->buf) ts->PrintString("[memory error]\n");
		}
		strncpy((char*)page->buf,(char*)pagebuf,page->length);
		parse_pop(p);
	}
	if (!(stricmp(el,"title") && stricmp(el,"head")
	        && stricmp(el,"pre") && stricmp(el,"html")))
	{
		parse_pop(p);
	}

}  /* End of end_hndl */

void proc_hndl(void *data, const char *target, const char *pidata)
{
}  /* End proc_hndl */

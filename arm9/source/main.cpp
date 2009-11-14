/* 

dslibris - an ebook reader for the Nintendo DS.

 Copyright (C) 2007-2008 Ray Haleblian

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include <nds.h>
#include <fat.h>
#include <expat.h>
#include "types.h"
#include "App.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"
#include "main.h"
#include "parse.h"

App *app;

/*---------------------------------------------------------------------------*/

int main(void)
{
	defaultExceptionHandler();
	
	// ARM7 initialization.
	
	powerON(POWER_ALL);
	//powerSET(POWER_LCD|POWER_2D_A|POWER_2D_B);
	irqInit();
	irqEnable(IRQ_VBLANK);
	//irqEnable(IRQ_VCOUNT);
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	
	// Get a console going.
	
	videoSetMode(0);
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	vramSetBankC(VRAM_C_SUB_BG);
	SUB_BG0_CR = BG_MAP_BASE(31);
	BG_PALETTE_SUB[255] = RGB15(15,31,15);
	consoleInitDefault(
		(u16*)SCREEN_BASE_BLOCK_SUB(31),
		(u16*)CHAR_BASE_BLOCK_SUB(0),16);
	iprintf("$ dslibris\n");
			
	app = new App();
	return app->Run();
}

u8 GetParserFace(parsedata_t *pdata)
{
	if (pdata->italic)
		return TEXT_STYLE_ITALIC;
	else if (pdata->bold)
		return TEXT_STYLE_BOLD;
	else
		return TEXT_STYLE_NORMAL;
}


bool iswhitespace(u8 c)
{
	switch (c)
	{
	case ' ':
	case '\t':
	case '\n':
		return true;
		break;
	default:
		return false;
		break;
	}
}

void prefs_start_hndl(	void *data,
						const XML_Char *name,
						const XML_Char **attr)
{
	parsedata_t* p = (parsedata_t*)data;
	int position = 0; //! Page position in book.
	char filename[MAXPATHLEN];
	bool current = FALSE;
	int i;
	
	if (!stricmp(name,"library"))
	{
		for(i=0;attr[i];i+=2)
			if(!stricmp(attr[i],"modtime"))
			   app->prefs->modtime = atoi(attr[i+1]);
	}
	else if (!stricmp(name,"library"))
	{
		for(i=0;attr[i];i+=2) {
			if(!strcmp(attr[i],"folder"))
				app->bookdir = std::string(attr[i+1]);
		}
	}
	else if (!stricmp(name,"screen"))
	{
		for(i=0;attr[i];i+=2)
		{
			if(!strcmp(attr[i],"brightness"))
			{
				app->brightness = atoi(attr[i+1]);
				app->brightness = app->brightness % 4;
			}
			else if(!strcmp(attr[i],"invert"))
			{
				app->ts->SetInvert(atoi(attr[i+1]));
			}
			else if(!strcmp(attr[i],"flip"))
			{
				app->orientation = atoi(attr[i+1]);
			}
		}
	}
	else if (!stricmp(name,"paragraph"))
	{
		for(i=0;attr[i];i+=2)
		{
			if(!strcmp(attr[i],"spacing")) app->paraspacing = atoi(attr[i+1]);
			if(!strcmp(attr[i],"indent")) app->paraindent = atoi(attr[i+1]);
		}
	}
	else if (!stricmp(name,"font"))
	{
		for(i=0;attr[i];i+=2)
		{
			if(!strcmp(attr[i],"size"))
				app->ts->pixelsize = atoi(attr[i+1]);
			else if(!strcmp(attr[i],"normal"))
				app->ts->SetFontFile((char *)attr[i+1], TEXT_STYLE_NORMAL);
			else if(!strcmp(attr[i],"bold"))
				app->ts->SetFontFile((char *)attr[i+1], TEXT_STYLE_BOLD);
			else if(!strcmp(attr[i],"italic"))
				app->ts->SetFontFile((char *)attr[i+1], TEXT_STYLE_ITALIC);
			else if (!strcmp(attr[i], "path")) {
				if (strlen(attr[i+1]))
					app->fontdir = string(attr[i+1]);
			}
		}
	}
	else if (!stricmp(name, "books"))
	{
		for (i = 0; attr[i]; i+=2) {
			if (!strcmp(attr[i], "reopen"))
				// For prefs where reopen was a string, reopen will get turned off.
				app->reopen = atoi(attr[i+1]);
			else if (!strcmp(attr[i], "path")) {
				if (strlen(attr[i+1]))
					app->bookdir = string(attr[i+1]);
			}
        }
	}
	else if (!stricmp(name, "book"))
	{
		strcpy(filename,"");
		current = FALSE;
		position = 0;
		for (i = 0; attr[i]; i+=2) {
			if (!strcmp(attr[i], "file"))
				strcpy(filename, attr[i+1]);
			if (!strcmp(attr[i], "page"))
				position = atoi(attr[i+1]);
			if (!strcmp(attr[i], "current"))
			{
				// Should warn if multiple books are current...
				// the last current book will win.
				if(atoi(attr[i+1])) current = TRUE;
			}
        }
		
		// Find the book index for this library entry
		// and set context for later bookmarks.
		vector<Book*>::iterator it;
		for(it = app->books.begin(); it < app->books.end(); it++)
		{
			const char *bookname = (*it)->GetFileName();
			if(!stricmp(bookname, filename))
			{
				// bookmark tags will refer to this.
				p->book = *it;
				
				char msg[128];
				sprintf(msg,"info : matched extant book '%s'.\n",bookname);
				app->Log(msg);
				
				if (current)
				{
					// Set this book as current.
					app->bookcurrent = *it;
					app->bookselected = *it;
				}
				if (position)
					// Set current page in this book.
					(*it)->SetPosition(position - 1);

				break;
			}
		}
	}
	else if (!stricmp(name, "bookmark"))
	{
		for (i = 0; attr[i]; i+=2) {
			if (!strcmp(attr[i], "page"))
				position = atoi(attr[i+1]);
		}
		
		if (p->book)
		{
			p->book->GetBookmarks()->push_back(position - 1);
		}
	}
	else if (!stricmp(name,"margin"))
	{
		for (i=0;attr[i];i+=2)
		{
			if (!strcmp(attr[i],"left")) app->ts->margin.left = atoi(attr[i+1]);
			if (!strcmp(attr[i],"right")) app->ts->margin.right = atoi(attr[i+1]);
			if (!strcmp(attr[i],"top")) app->ts->margin.top = atoi(attr[i+1]);
			if (!strcmp(attr[i],"bottom")) app->ts->margin.bottom = atoi(attr[i+1]);
		}
	}
	else if (!strcmp(name,"option"))
	{
		for (i=0;attr[i];i+=2)
		{
			if (!strcmp(attr[i],"swapshoulder")) 
				p->prefs->swapshoulder = atoi(attr[i+1]);
		}
	}
}

void prefs_end_hndl(void *data, const char *name)
{
	parsedata_t *p = (parsedata_t*)data;
	if (!stricmp(name,"book")) p->book = NULL;
}

int unknown_hndl(void *encodingHandlerData,
                 const XML_Char *name,
                 XML_Encoding *info)
{
	return(XML_STATUS_ERROR);
}

void default_hndl(void *data, const XML_Char *s, int len)
{
#ifdef DEBUG
	char msg[256];
	strncpy(msg,(const char*)s, len > 255 ? 255 : len);
	app->Log("info : ");
	app->Log(msg);
	app->Log("\n");
#endif

	parsedata_t *p = (parsedata_t*)data;
	if (s[0] == '&')
	{
		/** if it's decimal, convert the UTF-16 to UTF-8. */
		int code=0;
		sscanf(s,"&#%d;",&code);
		if (code)
		{
			if (code>=128 && code<=2047)
			{
				p->buf[p->buflen++] = 192 + (code/64);
				p->buf[p->buflen++] = 128 + (code%64);
			}
			else if (code>=2048 && code<=65535)
			{
				p->buf[p->buflen++] = 224 + (code/4096);
				p->buf[p->buflen++] = 128 + ((code/64)%64);
				p->buf[p->buflen++] = 128 + (code%64);
			}
			// TODO - support 4-byte codes
			
			p->pen.x += app->ts->GetAdvance(code, GetParserFace(p));
			return;
		}

		/** otherwise, handle only common HTML named entities. */
		if (!strnicmp(s,"&nbsp;",5))
		{
			p->buf[p->buflen++] = ' ';
			p->pen.x += app->ts->GetAdvance(' ', GetParserFace(p));
			return;
		}
		if (!stricmp(s,"&quot;"))
		{
			p->buf[p->buflen++] = '"';
			p->pen.x += app->ts->GetAdvance(' ', GetParserFace(p));
			return;
		}
		if (!stricmp(s,"&amp;"))
		{
			p->buf[p->buflen++] = '&';
			p->pen.x += app->ts->GetAdvance(' ', GetParserFace(p));
			return;
		}
		if (!stricmp(s,"&lt;"))
		{
			p->buf[p->buflen++] = '<';
			p->pen.x += app->ts->GetAdvance(' ', GetParserFace(p));
			return;
		}
		if (!stricmp(s,"&lt;"))
		{
			p->buf[p->buflen++] = '>';
			p->pen.x += app->ts->GetAdvance(' ', GetParserFace(p));
			return;
		}
	}
	
	// FIXME if we go more than an HTML entity passed in, we've lost he remainder!
	
}  /* End default_hndl */

static char title[32];

void title_start_hndl(void *userdata, const char *el, const char **attr)
{
	if(!stricmp(el,"title"))
	{
		app->parse_push((parsedata_t *)userdata,TAG_TITLE);
		strcpy(title,"");
	}
}

void title_char_hndl(void *userdata, const char *txt, int txtlen)
{
	if(strlen(title) > 27) return;
	if (!app->parse_in((parsedata_t*)userdata,TAG_TITLE)) return;

	for(u8 t=0;t<txtlen;t++)
	{
		if(iswhitespace(txt[t])) 
		{
			if(strlen(title)) strncat(title," ",1);
		}
		else strncat(title,txt+t,1);

		if (strlen(title) > 27)
		{
			strcat(title+27, "...");
			break;
		}
	}
}

void title_end_hndl(void *userdata, const char *el)
{
	parsedata_t *data = (parsedata_t*)userdata;
	if(!stricmp(el,"title")) data->book->SetTitle(title);
	if(!stricmp(el,"head")) data->status = 1; // done.
	app->parse_pop(data);	
}

void start_hndl(void *data, const char *el, const char **attr)
{
	parsedata_t *p = (parsedata_t*)data;
	if (!stricmp(el,"html")) app->parse_push(p,TAG_HTML);
	else if (!stricmp(el,"body")) app->parse_push(p,TAG_BODY);
	else if (!stricmp(el,"div")) app->parse_push(p,TAG_DIV);
	else if (!stricmp(el,"dt")) app->parse_push(p,TAG_DT);
	else if (!stricmp(el,"h1")) {
		app->parse_push(p,TAG_H1);
		p->buf[p->buflen] = TEXT_BOLD_ON;
		p->buflen++;
		p->pos++;
		p->bold = true;
	}
	else if (!stricmp(el,"h2")) app->parse_push(p,TAG_H2);
	else if (!stricmp(el,"h3")) app->parse_push(p,TAG_H3);
	else if (!stricmp(el,"h4")) app->parse_push(p,TAG_H4);
	else if (!stricmp(el,"h5")) app->parse_push(p,TAG_H5);
	else if (!stricmp(el,"h6")) app->parse_push(p,TAG_H6);
	else if (!stricmp(el,"head")) app->parse_push(p,TAG_HEAD);
	else if (!stricmp(el,"ol")) app->parse_push(p,TAG_OL);
	else if (!stricmp(el,"p")) app->parse_push(p,TAG_P);
	else if (!stricmp(el,"pre")) app->parse_push(p,TAG_PRE);
	else if (!stricmp(el,"script")) app->parse_push(p,TAG_SCRIPT);
	else if (!stricmp(el,"style")) app->parse_push(p,TAG_STYLE);
	else if (!stricmp(el,"title")) app->parse_push(p,TAG_TITLE);
	else if (!stricmp(el,"td")) app->parse_push(p,TAG_TD);
	else if (!stricmp(el,"ul")) app->parse_push(p,TAG_UL);
	else if (!stricmp(el,"strong") || !stricmp(el, "b")) {
		app->parse_push(p,TAG_STRONG);
		p->buf[p->buflen] = TEXT_BOLD_ON;
		p->buflen++;
		p->pos++;
		p->bold = true;
	}
	else if (!stricmp(el,"em") || !stricmp(el, "i")) {
		app->parse_push(p,TAG_EM);
		p->buf[p->buflen] = TEXT_ITALIC_ON;
		p->buflen++;
		p->italic = true;
	}
	else app->parse_push(p,TAG_UNKNOWN);
}  /* End of start_hndl */


void char_hndl(void *data, const XML_Char *txt, int txtlen)
{
	/** reflow text on the fly, into page data structure. **/

	parsedata_t *p = (parsedata_t *)data;
	if (app->parse_in(p,TAG_TITLE)) return;
	if (app->parse_in(p,TAG_SCRIPT)) return;
	if (app->parse_in(p,TAG_STYLE)) return;

	Text *ts = app->ts;
	if (p->buflen == 0)
	{
		/** starting a new page. **/
		p->pen.x = ts->margin.left;
		p->pen.y = ts->margin.top + ts->GetHeight();
		p->linebegan = false;
	}

	u8 advance=0;
	int i=0, j=0;
	while (i<txtlen)
	{
		if (txt[i] == '\r')
		{
			i++;
			continue;
		}

		if (iswhitespace(txt[i]))
		{
			if(app->parse_in(p,TAG_PRE))
			{
				p->buf[p->buflen++] = txt[i];
				if(txt[i] == '\n')
				{
					p->pen.x = ts->margin.left;
					p->pen.y += (ts->GetHeight() + ts->linespacing);
				}
				else {
					p->pen.x += ts->GetAdvance((u16)' ', GetParserFace(p));
				}
			}
			else if(p->linebegan && p->buflen
				&& !iswhitespace(p->buf[p->buflen-1]))
			{
				p->buf[p->buflen++] = ' ';
				p->pen.x += ts->GetAdvance((u16)' ', GetParserFace(p));	
			}
			i++;
		}
		else
		{
			p->linebegan = true;
			advance = 0;
			u8 bytes = 1;
			for (j=i;(j<txtlen) && (!iswhitespace(txt[j]));j+=bytes)
			{
				/** set type until the end of the next word.
				    account for UTF-8 characters when advancing. **/
				u32 code;
				if (txt[j] > 127)
					bytes = ts->GetCharCode((char*)&(txt[j]),&code);
				else
				{
					code = txt[j];
					bytes = 1;
				}

				advance += ts->GetAdvance(code, GetParserFace(p));
					
				if(advance > ts->display.width - ts->margin.right - ts->margin.left)
				{
					// here's a line-long word, need to break it now.
					break;
				}
			}
		}

		if ((p->pen.x + advance) > (ts->display.width - ts->margin.right))
		{
			// we overran the margin, insert a break.
			p->buf[p->buflen++] = '\n';
			p->pen.x = ts->margin.left;
			p->pen.y += (ts->GetHeight() + ts->linespacing);
			p->linebegan = false;
		}

		if (p->pen.y > (ts->display.height - ts->margin.bottom))
		{
			// reached bottom of screen.
			if(p->screen == 1)
			{
				// page full.
				// put chars into current page.
				Page *page = p->book->AppendPage();
				page->SetBuffer(p->buf, p->buflen);
				page->start = p->pos;
				p->pos += p->buflen;
				page->end = p->pos;

				// make a new page.
				p->buflen = 0;
				if (p->italic) p->buf[p->buflen++] = TEXT_ITALIC_ON;
				if (p->bold) p->buf[p->buflen++] = TEXT_BOLD_ON;
				p->screen = 0;
			}
			else 
				// move to right screen.
				p->screen = 1;

			p->pen.x = ts->margin.left;
			p->pen.y = ts->margin.top + ts->GetHeight();
		}

		/** append this word to the page.
			chars stay UTF-8 until they are rendered. **/

		for (;i<j;i++)
		{
			if (iswhitespace(txt[i]))
			{
				if (p->linebegan)
				{
					p->buf[p->buflen] = ' ';
					p->buflen++;
				}
			}
			else
			{
				p->linebegan = true;
				p->buf[p->buflen] = txt[i];
				p->buflen++;
			}
		}
		p->pen.x += advance;
		advance = 0;
	}
}  /* End char_hndl */

void end_hndl(void *data, const char *el)
{
	parsedata_t *p = (parsedata_t *)data;
	Text *ts = app->ts;	
	if (
	       !stricmp(el,"br")
	    || !stricmp(el,"div")
	    || !stricmp(el,"dt")
	    || !stricmp(el,"h1")
	    || !stricmp(el,"h2")
	    || !stricmp(el,"h3")
	    || !stricmp(el,"h4")
	    || !stricmp(el,"h5")
	    || !stricmp(el,"h6")
	    || !stricmp(el,"hr")
	    || !stricmp(el,"li")
	    || !stricmp(el,"p")
	    || !stricmp(el,"pre")
	    || !stricmp(el,"ol")
	    || !stricmp(el,"td")
	    || !stricmp(el,"ul"))
	{
		if(p->linebegan || !strcmp(el,"br")) {
			p->linebegan = false;
			p->buf[p->buflen] = '\n';
			p->buflen++;
			p->pen.x = MARGINLEFT;
			p->pen.y += ts->GetHeight() + ts->linespacing;
			if (!stricmp(el,"p"))
			{
				for(int i=0;i<app->paraspacing;i++)
				{
					p->buf[p->buflen++] = '\n';
					p->pen.x = MARGINLEFT;
					p->pen.y += ts->GetHeight() + ts->linespacing;
				}
				for(int i=0;i<app->paraindent;i++)
				{
					p->buf[p->buflen++] = ' ';
					p->pen.x += ts->GetAdvance(' ', GetParserFace(p));
				}
			}
			else if(!strcmp(el,"h1")) {
				p->buf[p->buflen] = '\n';
				p->buflen++;
				p->linebegan = false;
				p->pen.x = ts->margin.left;
				p->pen.y += ts->GetHeight() + ts->linespacing;
				p->buf[p->buflen] = TEXT_BOLD_OFF;
				p->buflen++;
				p->bold = false;
			} else if (
				   !strcmp(el,"h2")
				|| !strcmp(el,"h3")
				|| !strcmp(el,"h4")
				|| !strcmp(el,"h5")
				|| !strcmp(el,"h6")
				|| !strcmp(el,"hr")
				|| !strcmp(el,"pre")
			)
			{
				p->buf[p->buflen] = '\n';
				p->buflen++;
				p->linebegan = false;
				p->pen.x = ts->margin.left;
				p->pen.y += ts->GetHeight() + ts->linespacing;
			}
			if (p->pen.y > (ts->display.height - ts->margin.bottom))
			{
				if (p->screen == 1)
				{
					// End of right screen; end of page.
					// Copy in buffered char data into a new page.
					Page *page = p->book->AppendPage();
					page->SetBuffer(p->buf, p->buflen);
					p->buflen = 0;
					if (p->italic) p->buf[p->buflen++] = TEXT_ITALIC_ON;
					if (p->bold )p->buf[p->buflen++] = TEXT_BOLD_ON;
					p->screen = 0;
				}
				else
					// End of left screen; same page, next screen.
					p->screen = 1;
				p->pen.x = ts->margin.left;
				p->pen.y = ts->margin.top + ts->GetHeight();
				p->linebegan = false;
			}
		}
	} else if (!stricmp(el,"body")) {
		// Save off our last page.
		Page *page = p->book->AppendPage();
		page->SetBuffer(p->buf,p->buflen);
		p->buflen = 0;
		if (p->italic) p->buf[p->buflen++] = TEXT_ITALIC_ON;
		if (p->bold )p->buf[p->buflen++] = TEXT_BOLD_ON;
	} else if (!stricmp(el, "strong") || !stricmp(el, "b")) {
		p->buf[p->buflen] = TEXT_BOLD_OFF;
		p->buflen++;
		p->bold = false;
	} else if (!stricmp(el, "em") || !stricmp(el, "i")) {
		p->buf[p->buflen] = TEXT_ITALIC_OFF;
		p->buflen++;
		p->italic = false;
	}

	app->parse_pop(p);
}  /* End of end_hndl */

void proc_hndl(void *data, const char *target, const char *pidata)
{
}  /* End proc_hndl */

void parse_init(parsedata_t *data)
{
	data->stacksize = 0;
	data->pos = 0;
	data->book = NULL;
	data->prefs = NULL;
	data->screen = 0;
	data->pen.x = 0;
	data->pen.y = 0;
	data->linebegan = false;
	data->bold = false;
	data->italic = false;
	strcpy((char*)data->buf,"");
	data->buflen = 0;
	data->status = 0;
}

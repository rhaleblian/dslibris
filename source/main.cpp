/* 

dslibris - an ebook reader for the Nintendo DS.

 Copyright (C) 2007-2020 Ray Haleblian (ray@haleblian.com)

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

#include <sys/param.h>
#include "dirent.h"
#include "fat.h"

#include "app.h"
#include "book.h"
#include "button.h"
#include "text.h"
#include "expat.h"
#include "main.h"
#include "parse.h"
#include "types.h"

App *app;
char msg[256];
int ft_main(int argc, char **argv);

/*---------------------------------------------------------------------------*/


int kungheyfatcheck(void) {
	iprintf("** kungheyfatcheck **\n");
	iprintf("root directory:\n");
	swiWaitForVBlank();

	DIR *dp = opendir("/");
	if (!dp) {
		printf("[PANIC] root dir inaccessible!\n");
		return false;
	}
	struct dirent *ent;
	while ((ent = readdir(dp)))
	{
		iprintf("%s %d\n", ent->d_name, ent->d_type);
	}
	closedir(dp);

	return true;
}

PrintConsole* boot_console(void) {
	// Get a console going.
	auto console = consoleDemoInit();
	if (!console) iprintf("[FAIL] console!\n");		// This, of course, won't print :D
	else iprintf("[OK] console\n");
	return console;
}

int boot_filesystem(void) {
	// Start up the filesystem.
	bool success = fatInitDefault();
	if (!success) iprintf("[FAIL] filesystem!\n");
	else iprintf("[OK] filesystem\n");
	return success;
}

void spin(void) {
	while(true) swiWaitForVBlank();
}

void fatal(const char *msg) {
	iprintf(msg);
	spin();
}

int main(void)
{
	if(!boot_console()) spin();
	if(!boot_filesystem()) spin();

	//kungheyfatcheck();
	// asciiart();
	// while(true) swiWaitForVBlank();

	app = new App();
	return app->Run();
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
	data->pagecount = 0;
}

u8 GetParserFace(parsedata_t *pdata)
{
	if (pdata->italic)
		return TEXT_STYLE_ITALIC;
	else if (pdata->bold)
		return TEXT_STYLE_BOLD;
	else
		return TEXT_STYLE_REGULAR;
}

void WriteBufferToCache(parsedata_t *pdata)
{
	// Only cache if we are laying out text.
	//	if (pdata->cachefile && pdata->status) {
	//		fwrite(pdata->buf, 1, pdata->buflen, pdata->cachefile);
	//}
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
	
	if (!strcmp(name,"library"))
	{
		for(i=0;attr[i];i+=2)
			if(!strcmp(attr[i],"modtime"))
			   app->prefs->modtime = atoi(attr[i+1]);
	}
	else if (!strcmp(name,"library"))
	{
		for(i=0;attr[i];i+=2) {
			if(!strcmp(attr[i],"folder"))
				app->bookdir = std::string(attr[i+1]);
		}
	}
	else if (!strcmp(name,"screen"))
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
	else if (!strcmp(name,"paragraph"))
	{
		for(i=0;attr[i];i+=2)
		{
			if(!strcmp(attr[i],"spacing")) app->paraspacing = atoi(attr[i+1]);
			if(!strcmp(attr[i],"indent")) app->paraindent = atoi(attr[i+1]);
		}
	}
	else if (!strcmp(name,"font"))
	{
		for(i=0;attr[i];i+=2)
		{
			if(!strcmp(attr[i],"size"))
				app->ts->pixelsize = atoi(attr[i+1]);
			else if(!strcmp(attr[i],"normal"))
				app->ts->SetFontFile((char *)attr[i+1], TEXT_STYLE_REGULAR);
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
	else if (!strcmp(name, "books"))
	{
		for (i = 0; attr[i]; i+=2) {
			if (!strcmp(attr[i], "reopen"))
				// For prefs where reopen was a string,
				// reopen will get turned off.
				app->reopen = atoi(attr[i+1]);
			else if (!strcmp(attr[i], "path")) {
				if (strlen(attr[i+1]))
					app->bookdir = string(attr[i+1]);
			}
        }
	}
	else if (!strcmp(name, "book"))
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
			if(!strcmp(bookname, filename))
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
	else if (!strcmp(name, "bookmark"))
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
	else if (!strcmp(name,"margin"))
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
	//! Exit element callback for the prefs file.
	parsedata_t *p = (parsedata_t*)data;
	if (!strcmp(name,"book")) p->book = NULL;
}

int unknown_hndl(void *encodingHandlerData,
                 const XML_Char *name,
                 XML_Encoding *info)
{
	// noops!
	strcpy(msg, "warn : encoding handler encountered, did nothing.");
	app->Log(msg);
	return 0;
}

void default_hndl(void *data, const XML_Char *s, int len)
{
	//! Fallback callback. NYUK!
	
#ifdef DEBUG
	char msg[256];
	strncpy(msg,(const char*)s, len > 255 ? 255 : len);
	app->Log("info : ");
	app->Log(msg);
	app->Log("\n");
#endif

	int advancespace = app->ts->GetAdvance(' ');
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
			
			p->pen.x += app->ts->GetAdvance(code);
			return;
		}

		/** otherwise, handle only common HTML named entities. */
		if (!strncmp(s,"&nbsp;",5))
		{
			p->buf[p->buflen++] = ' ';
			p->pen.x += advancespace;
			return;
		}
		if (!strcmp(s,"&quot;"))
		{
			p->buf[p->buflen++] = '"';
			p->pen.x += advancespace;
			return;
		}
		if (!strcmp(s,"&amp;"))
		{
			p->buf[p->buflen++] = '&';
			p->pen.x += advancespace;
			return;
		}
		if (!strcmp(s,"&lt;"))
		{
			p->buf[p->buflen++] = '<';
			p->pen.x += advancespace;
			return;
		}
		if (!strcmp(s,"&lt;"))
		{
			p->buf[p->buflen++] = '>';
			p->pen.x += advancespace;
			return;
		}
	}
}

static char title[32];

void title_start_hndl(void *userdata, const char *el, const char **attr)
{
	//! Expat callback, when entering an element.
	//! For finding book title only.

	if(!strcmp(el,"title"))
	{
		app->parse_push((parsedata_t *)userdata,TAG_TITLE);
		strcpy(title,"");
	}
}

void title_char_hndl(void *userdata, const char *txt, int txtlen)
{
	//! Expat callback, when in char data for element.
	//! For finding book title only.
	if(strlen(title) > 27) return;
	if (!app->parse_in((parsedata_t*)userdata,TAG_TITLE)) return;

	for(u8 t=0;t<txtlen;t++)
	{
		if(iswhitespace(txt[t])) 
		{
			if(strlen(title)) strncat(title," ",2);
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
	//! Expat callback, when exiting an element.
	//! For finding book title only.
	parsedata_t *data = (parsedata_t*)userdata;
	if(!strcmp(el,"title")) data->book->SetTitle(title);
	if(!strcmp(el,"head")) data->status = 1; // done.
	app->parse_pop(data);
}

// Expat callbacks for parsing full text follow. //

void linefeed(parsedata_t *p) {
	Text *ts = app->ts;
	p->buf[p->buflen++] = '\n';
	p->pen.x = MARGINLEFT;
	p->pen.y += ts->GetHeight() + ts->linespacing;
	p->linebegan = false;
}

bool blankline(parsedata_t *p) {
	// Was the preceding text a blank line?
	if (p->buflen < 3) return true;
	return (p->buf[p->buflen-1] == '\n') && (p->buf[p->buflen-2] == '\n');
}

void start_hndl(void *data, const char *el, const char **attr)
{
	//! Expat callback, when starting an element.

	parsedata_t *p = (parsedata_t*)data;

	if (!strcmp(el,"html")) app->parse_push(p,TAG_HTML);
	else if (!strcmp(el,"body")) app->parse_push(p,TAG_BODY);
	else if (!strcmp(el,"div")) app->parse_push(p,TAG_DIV);
	else if (!strcmp(el,"dt")) app->parse_push(p,TAG_DT);
	else if (!strcmp(el,"h1")) {
		app->parse_push(p,TAG_H1);
		bool lf = !blankline(p);
		p->buf[p->buflen] = TEXT_BOLD_ON;
		p->buflen++;
		p->pos++;
		p->bold = true;
		if (lf) linefeed(p);
	}
	else if (!strcmp(el,"h2")) {
		app->parse_push(p,TAG_H2);
		bool lf = !blankline(p);
		p->buf[p->buflen] = TEXT_BOLD_ON;
		p->buflen++;
		p->pos++;
		p->bold = true;
		if (lf) linefeed(p);		
	}
	else if (!strcmp(el,"h3")) {
		app->parse_push(p,TAG_H3);
		linefeed(p);
	}
	else if (!strcmp(el,"h4")) {
		app->parse_push(p,TAG_H4);
		if (!blankline(p)) linefeed(p);
	}
	else if (!strcmp(el,"h5")) {
		app->parse_push(p,TAG_H5);
		if (!blankline(p)) linefeed(p);
	}
	else if (!strcmp(el,"h6")) {
		app->parse_push(p,TAG_H6);
		if (!blankline(p)) linefeed(p);
	}
	else if (!strcmp(el,"head")) app->parse_push(p,TAG_HEAD);
	else if (!strcmp(el,"ol")) app->parse_push(p,TAG_OL);
	else if (!strcmp(el,"p")) {
		app->parse_push(p,TAG_P);
		if (!blankline(p)) {
			// for(int i=0;i<app->paraspacing;i++)
			// {
			// 	linefeed(p);
			// }
			// for(int i=0;i<app->paraindent;i++)
			// {
			// 	p->buf[p->buflen++] = ' ';
			// 	p->pen.x += ts->GetAdvance(' ');
			// }
			linefeed(p);
		}
	}
	else if (!strcmp(el,"pre")) app->parse_push(p,TAG_PRE);
	else if (!strcmp(el,"script")) app->parse_push(p,TAG_SCRIPT);
	else if (!strcmp(el,"style")) app->parse_push(p,TAG_STYLE);
	else if (!strcmp(el,"title")) app->parse_push(p,TAG_TITLE);
	else if (!strcmp(el,"td")) app->parse_push(p,TAG_TD);
	else if (!strcmp(el,"ul")) app->parse_push(p,TAG_UL);
	else if (!strcmp(el,"strong") || !strcmp(el, "b")) {
		app->parse_push(p,TAG_STRONG);
		p->buf[p->buflen] = TEXT_BOLD_ON;
		p->buflen++;
		p->pos++;
		p->bold = true;
	}
	else if (!strcmp(el,"em") || !strcmp(el, "i")) {
		app->parse_push(p,TAG_EM);
		p->buf[p->buflen] = TEXT_ITALIC_ON;
		p->buflen++;
		p->italic = true;
	}
	else app->parse_push(p,TAG_UNKNOWN);
}

void char_hndl(void *data, const XML_Char *txt, int txtlen)
{
	//! reflow text on the fly, into page data structure.
	
	parsedata_t *p = (parsedata_t *)data;
	//if (p->pagecount > 3) return;
	if (app->parse_in(p,TAG_TITLE)) return;
	if (app->parse_in(p,TAG_SCRIPT)) return;
	if (app->parse_in(p,TAG_STYLE)) return;

	Text *ts = app->ts;
	int lineheight = ts->GetHeight();
	int linespacing = ts->linespacing;
	int spaceadvance = ts->GetAdvance((u16)' ');

	if (p->buflen == 0)
	{
		/** starting a new page. **/
		p->pen.x = ts->margin.left;
		p->pen.y = ts->margin.top + lineheight;
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
					p->pen.y += (lineheight + linespacing);
				}
				else {
					p->pen.x += spaceadvance;
				}
			}
			else if(p->linebegan && p->buflen
				&& !iswhitespace(p->buf[p->buflen-1]))
			{
				p->buf[p->buflen++] = ' ';
				p->pen.x += spaceadvance;				
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
				u32 code = txt[j];
				bytes = 1;
				if (code >> 7) {
					// FIXME the performance bottleneck					
					bytes = ts->GetCharCode((char*)&(txt[j]),&code);
				}

				advance += ts->GetAdvance(code);
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
			p->pen.y += (lineheight + linespacing);
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
				p->pagecount++;

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
			p->pen.y = ts->margin.top + lineheight;
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
	
	if (!strcmp(el,"body")) {
		// Save off our last page.
		Page *page = p->book->AppendPage();
		page->SetBuffer(p->buf,p->buflen);
		if (app->cache)
			WriteBufferToCache(p);
		p->buflen = 0;
		// Retain styles across the page.
		if (p->italic) p->buf[p->buflen++] = TEXT_ITALIC_ON;
		if (p->bold) p->buf[p->buflen++] = TEXT_BOLD_ON;
		app->parse_pop(p);
		return;
	}
	
	if (!strcmp(el,"br")) {
		linefeed(p);
	} else if (!strcmp(el,"p")) {
		linefeed(p);
		linefeed(p);
	} else if(!strcmp(el,"div")) {
	} else if (!strcmp(el, "strong") || !strcmp(el, "b")) {
		p->buf[p->buflen] = TEXT_BOLD_OFF;
		p->buflen++;
		p->bold = false;
	} else if (!strcmp(el, "em") || !strcmp(el, "i")) {
		p->buf[p->buflen] = TEXT_ITALIC_OFF;
		p->buflen++;
		p->italic = false;
	} else if(!strcmp(el,"h1")) {
		p->buf[p->buflen] = TEXT_BOLD_OFF;
		p->buflen++;
		p->bold = false;
		linefeed(p);
		linefeed(p);
	} else if (!strcmp(el,"h2")) {
		p->buf[p->buflen] = TEXT_BOLD_OFF;
		p->buflen++;
		p->bold = false;
		linefeed(p);
	} else if (
		   !strcmp(el,"h3")
		|| !strcmp(el,"h4")
		|| !strcmp(el,"h5")
		|| !strcmp(el,"h6")
		|| !strcmp(el,"hr")
		|| !strcmp(el,"pre")
	) {
		linefeed(p);
		linefeed(p);
	} else if (
		   !strcmp(el, "li")
		|| !strcmp(el, "ul")
		|| !strcmp(el, "ol")
	) {
		linefeed(p);
	}

	app->parse_pop(p);

	if (p->pen.y > (ts->display.height - ts->margin.bottom))
	{
		if (p->screen == 1)
		{
			// End of right screen; end of page.
			// Copy in buffered char data into a new page.
			Page *page = p->book->AppendPage();
			page->SetBuffer(p->buf, p->buflen);
			if (app->cache)
				WriteBufferToCache(p);
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

	app->parse_pop(p);
}

void proc_hndl(void *data, const char *target, const char *pidata)
{
	app->Log("called proc_hndl().\n");
}

int getSize(uint8 *source, uint16 *dest, uint32 arg) {
       return *(uint32*)source;
}

uint8 readByte(uint8 *source) { return *source; }

void drawstack(u16 *screen) {
       TDecompressionStream decomp = {getSize, NULL, readByte};
       swiDecompressLZSSVram((void*)splashBitmap, screen, 0, &decomp);
}

#include <expat.h>
#include <nds.h>
#include "app.h"
#include "book.h"
#include "define.h"
#include "page.h"
#include "prefs.h"
#include "text.h"

#include "parse.h"

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
	data->app = NULL;
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
	App *app = p->app;
	Prefs *prefs = p->prefs;
	int position = 0; //! Page position in book.
	char filename[MAXPATHLEN];
	bool current = FALSE;
	int i;
	
	if (!strcmp(name,"library"))
	{
		for(i=0;attr[i];i+=2)
			if(!strcmp(attr[i],"modtime"))
			   prefs->modtime = atoi(attr[i+1]);
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
				app->SetBrightness(atoi(attr[i+1]));
			}
			else if(!strcmp(attr[i],"invert"))
			{
				app->ts->SetInvert(atoi(attr[i+1]));
			}
			else if(!strcmp(attr[i],"flip"))
			{
				app->ts->orientation = atoi(attr[i+1]);
			}
		}
	}
	else if (!strcmp(name,"paragraph"))
	{
		for(i=0;attr[i];i+=2)
		{
			// if(!strcmp(attr[i],"spacing")) app->paraspacing = atoi(attr[i+1]);
			// if(!strcmp(attr[i],"indent")) app->paraindent = atoi(attr[i+1]);
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
					app->fontdir = std::string(attr[i+1]);
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
					app->bookdir = std::string(attr[i+1]);
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
		std::vector<Book*>::iterator it;
		for(it = app->books.begin(); it < app->books.end(); it++)
		{
			const char *bookname = (*it)->GetFileName();
			if(!strcmp(bookname, filename))
			{
				// bookmark tags will refer to this.
				p->book = *it;
				
				char msg[128];
				sprintf(msg,"info : matched extant book '%s'.\n",bookname);
				// app->Log(msg);
				
				if (current)
				{
					// Set this book as current.
					app->bookcurrent = *it;
					// app->bookselected = *it;
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
	return 0;
}

void default_hndl(void *data, const XML_Char *s, int len)
{
	//! Fallback callback. NYUK!
	parsedata_t *p = (parsedata_t*)data;
	App *app = p->app;
	int advancespace = app->ts->GetAdvance(' ');
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

	auto p = (parsedata_t *)userdata;
	if(!strcmp(el,"title"))
	{
		parse_push(p, TAG_TITLE);
		strcpy(title,"");
	}
}

void title_char_hndl(void *userdata, const char *txt, int txtlen)
{
	//! Expat callback, when in char data for element.
	//! For finding book title only.
	auto p = (parsedata_t*)userdata;

	if(strlen(title) > 27) return;
	if (!parse_in(p, TAG_TITLE)) return;

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
	parsedata_t *p = (parsedata_t*)userdata;
	if(!strcmp(el,"title")) p->book->SetTitle(title);
	if(!strcmp(el,"head")) p->status = 1; // done.
	parse_pop(p);
}

// Expat callbacks for parsing full text follow. //

void linefeed(parsedata_t *p) {
	Text *ts = p->app->ts;
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
	if (!strcmp(el,"html")) parse_push(p,TAG_HTML);
	else if (!strcmp(el,"body")) parse_push(p,TAG_BODY);
	else if (!strcmp(el,"div")) parse_push(p,TAG_DIV);
	else if (!strcmp(el,"dt")) parse_push(p,TAG_DT);
	else if (!strcmp(el,"h1")) {
		parse_push(p,TAG_H1);
		bool lf = !blankline(p);
		p->buf[p->buflen] = TEXT_BOLD_ON;
		p->buflen++;
		p->pos++;
		p->bold = true;
		if (lf) linefeed(p);
	}
	else if (!strcmp(el,"h2")) {
		parse_push(p,TAG_H2);
		bool lf = !blankline(p);
		p->buf[p->buflen] = TEXT_BOLD_ON;
		p->buflen++;
		p->pos++;
		p->bold = true;
		if (lf) linefeed(p);		
	}
	else if (!strcmp(el,"h3")) {
		parse_push(p,TAG_H3);
		linefeed(p);
	}
	else if (!strcmp(el,"h4")) {
		parse_push(p,TAG_H4);
		if (!blankline(p)) linefeed(p);
	}
	else if (!strcmp(el,"h5")) {
		parse_push(p,TAG_H5);
		if (!blankline(p)) linefeed(p);
	}
	else if (!strcmp(el,"h6")) {
		parse_push(p,TAG_H6);
		if (!blankline(p)) linefeed(p);
	}
	else if (!strcmp(el,"head")) parse_push(p,TAG_HEAD);
	else if (!strcmp(el,"ol")) parse_push(p,TAG_OL);
	else if (!strcmp(el,"p")) {
		parse_push(p,TAG_P);
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
	else if (!strcmp(el,"pre")) parse_push(p,TAG_PRE);
	else if (!strcmp(el,"script")) parse_push(p,TAG_SCRIPT);
	else if (!strcmp(el,"style")) parse_push(p,TAG_STYLE);
	else if (!strcmp(el,"title")) parse_push(p,TAG_TITLE);
	else if (!strcmp(el,"td")) parse_push(p,TAG_TD);
	else if (!strcmp(el,"ul")) parse_push(p,TAG_UL);
	else if (!strcmp(el,"strong") || !strcmp(el, "b")) {
		parse_push(p,TAG_STRONG);
		p->buf[p->buflen] = TEXT_BOLD_ON;
		p->buflen++;
		p->pos++;
		p->bold = true;
	}
	else if (!strcmp(el,"em") || !strcmp(el, "i")) {
		parse_push(p,TAG_EM);
		p->buf[p->buflen] = TEXT_ITALIC_ON;
		p->buflen++;
		p->italic = true;
	}
	else parse_push(p,TAG_UNKNOWN);
}

void char_hndl(void *data, const XML_Char *txt, int txtlen)
{
	//! reflow text on the fly, into page data structure.
	
	parsedata_t *p = (parsedata_t *)data;
	//if (p->pagecount > 3) return;
	if (parse_in(p,TAG_TITLE)) return;
	if (parse_in(p,TAG_SCRIPT)) return;
	if (parse_in(p,TAG_STYLE)) return;

	Text *ts = p->app->ts;
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
			if(parse_in(p,TAG_PRE))
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
	Text *ts = p->app->ts;	
	Book *book = p->book;

	if (!strcmp(el,"body")) {
		// Save off our last page.
		Page *page = book->AppendPage();
		page->SetBuffer(p->buf,p->buflen);
		// if (ts->cache)
		// 	WriteBufferToCache(p);
		p->buflen = 0;
		// Retain styles across the page.
		if (p->italic) p->buf[p->buflen++] = TEXT_ITALIC_ON;
		if (p->bold) p->buf[p->buflen++] = TEXT_BOLD_ON;
		parse_pop(p);
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

	parse_pop(p);

	if (p->pen.y > (ts->display.height - ts->margin.bottom))
	{
		if (p->screen == 1)
		{
			// End of right screen; end of page.
			// Copy in buffered char data into a new page.
			Page *page = p->book->AppendPage();
			page->SetBuffer(p->buf, p->buflen);
			// if (app->cache)
			// 	WriteBufferToCache(p);
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

	parse_pop(p);
}

void proc_hndl(void *data, const char *target, const char *pidata)
{
	// app->Log("called proc_hndl().\n");
}

void parse_get_error(XML_Parser p, char* msg)
{
	sprintf(msg,"%d:%d: %s\n",
		(int)XML_GetCurrentLineNumber(p),
		(int)XML_GetCurrentColumnNumber(p),
		XML_ErrorString(XML_GetErrorCode(p)));
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

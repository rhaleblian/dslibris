/** ---------------------------------------------------------------------------
dslibris - an ebook reader for Nintendo DS
<gpl />
--------------------------------------------------------------------------- **/

#include <nds.h>

#include <expat.h>
#include "types.h"
#include "App.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"
#include "main.h"
#include "parse.h"

App *app;
bool linebegan = false;

/*---------------------------------------------------------------------------*/

int main(void)
{
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

void prefs_start_hndl(	void *userdata,
						const XML_Char *name,
						const XML_Char **attr)
{
	Book *data = (Book*)userdata;
	char filename[64];
	strcpy(filename,"");
	u16 position = 0;
	if (!stricmp(name,"screen"))
	{
		for(int i=0;attr[i];i+=2)
		{
			if(!strcmp(attr[i],"brightness"))
			{
				app->brightness = atoi(attr[i+1]);
				app->brightness = app->brightness > 3 ? 3 : app->brightness;	
			}
			else if(!strcmp(attr[i],"invert"))
			{
				app->ts->SetInvert(atoi(attr[i+1]));
			}
		}
	}
	else if (!stricmp(name,"font"))
	{
		for(int i=0;attr[i];i+=2)
		{
			if(!strcmp(attr[i],"size"))
				app->ts->SetPixelSize(atoi(attr[i+1]));
		}
	}
	else if (!stricmp(name,"bookmark") || !stricmp(name,"book"))
	{
		u8 i;
		for (i=0;attr[i];i+=2)
		{
			if (!strcmp(attr[i],"file")) strcpy(filename, attr[i+1]);
			if (!strcmp(attr[i],"position")) position = atoi(attr[i+1]);
			if (!strcmp(attr[i],"page")) position = atoi(attr[i+1]);
		}
		for(i=0;i<app->bookcount;i++)
		{
			if(!stricmp(data[i].GetFileName(),filename))
			{
				if(position) data[i].SetPosition(position-1);
				if(!stricmp(name,"book")) app->bookcurrent = i;
				break;
			}
		}
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
		page_t *page = &(app->pages[app->pagecurrent]);

		/** handle only common iso-8859-1 character codes. */
		if (!strnicmp(s,"&nbsp;",5))
		{
			app->pagebuf[page->length++] = ' ';
			p->pen.x += app->ts->GetAdvance(' ');
			return;
		}

		/** if it's decimal, convert the UTF-16 to UTF-8. */
		int code=0;
		sscanf(s,"&#%d;",&code);
		if (code)
		{
			if (code>=128 && code<=2047)
			{
				app->pagebuf[page->length++] = 192 + (code/64);
				app->pagebuf[page->length++] = 128 + (code%64);
			}
			else if (code>=2048 && code<=65535)
			{
				app->pagebuf[page->length++] = 224 + (code/4096);
				app->pagebuf[page->length++] = 128 + ((code/64)%64);
				app->pagebuf[page->length++] = 128 + (code%64);
			}
			// TODO - support 4-byte codes
			
			p->pen.x += app->ts->GetAdvance(code);
		}
	}
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
		if(iswhitespace(txt[t])) strncat(title," ",1);
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
	app->parse_pop(data);	
}

void start_hndl(void *data, const char *el, const char **attr)
{
	parsedata_t *pdata = (parsedata_t*)data;
	if (!stricmp(el,"html")) app->parse_push(pdata,TAG_HTML);
	else if (!stricmp(el,"body")) app->parse_push(pdata,TAG_BODY);
	else if (!stricmp(el,"div")) app->parse_push(pdata,TAG_DIV);
	else if (!stricmp(el,"dt")) app->parse_push(pdata,TAG_DT);
	else if (!stricmp(el,"h1")) app->parse_push(pdata,TAG_H1);
	else if (!stricmp(el,"h2")) app->parse_push(pdata,TAG_H2);
	else if (!stricmp(el,"h3")) app->parse_push(pdata,TAG_H3);
	else if (!stricmp(el,"h4")) app->parse_push(pdata,TAG_H4);
	else if (!stricmp(el,"h5")) app->parse_push(pdata,TAG_H5);
	else if (!stricmp(el,"h6")) app->parse_push(pdata,TAG_H6);
	else if (!stricmp(el,"head")) app->parse_push(pdata,TAG_HEAD);
	else if (!stricmp(el,"ol")) app->parse_push(pdata,TAG_OL);
	else if (!stricmp(el,"p")) app->parse_push(pdata,TAG_P);
	else if (!stricmp(el,"pre")) app->parse_push(pdata,TAG_PRE);
	else if (!stricmp(el,"script")) app->parse_push(pdata,TAG_SCRIPT);
	else if (!stricmp(el,"style")) app->parse_push(pdata,TAG_STYLE);
	else if (!stricmp(el,"title")) app->parse_push(pdata,TAG_TITLE);
	else if (!stricmp(el,"td")) app->parse_push(pdata,TAG_TD);
	else if (!stricmp(el,"ul")) app->parse_push(pdata,TAG_UL);
	else app->parse_push(pdata,TAG_UNKNOWN);
}  /* End of start_hndl */


void char_hndl(void *data, const XML_Char *txt, int txtlen)
{
	/** reflow text on the fly, into page data structure. **/

	parsedata_t *pdata = (parsedata_t *)data;
	if (app->parse_in(pdata,TAG_SCRIPT)) return;
	if (app->parse_in(pdata,TAG_STYLE)) return;
	if (app->pagecount == MAXPAGES) return;

	page_t *page = &(app->pages[app->pagecurrent]);
	if (page->length == 0)
	{
		/** starting a new page. **/
		pdata->pen.x = MARGINLEFT;
		pdata->pen.y = MARGINTOP + app->ts->GetHeight();
		linebegan = false;
	}

	u8 advance=0;
	int i=0;
	while (i<txtlen)
	{
		if (txt[i] == '\r')
		{
			i++;
			continue;
		}

		if (iswhitespace(txt[i]))
		{
			if(linebegan)
			{
				app->pagebuf[page->length++] = ' ';
				pdata->pen.x += app->ts->GetAdvance((u16)' ');
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
				u32 code;
				if (txt[j] > 127)
					bytes = app->ts->GetCharCode((char*)&(txt[j]),&code);
				else
				{
					code = txt[j];
					bytes = 1;
				}
				advance += app->ts->GetAdvance(code);
				if(advance > PAGE_WIDTH-MARGINRIGHT-MARGINLEFT)
				{
					// here's a line-long word, need to break it now.
					break;
				}
			}

			/** reflow - if we overrun the margin, 
			insert a break. **/

			if ((pdata->pen.x + advance) > (PAGE_WIDTH-MARGINRIGHT))
			{
				app->pagebuf[page->length] = '\n';
				page->length++;
				pdata->pen.x = MARGINLEFT;
				pdata->pen.y += (app->ts->GetHeight() + LINESPACING);

				if (pdata->pen.y > (PAGE_HEIGHT-MARGINBOTTOM))
				{
					if (app->parse_pagefeed(pdata,page))
					{
						page++;
						app->page_init(page);
						app->pagecurrent++;
						app->pagecount++;
						if(app->pagecount == MAXPAGES) return;
					}
				}
				linebegan = false;
			}

			/** append this word to the page. to save space,
			chars will stay UTF-8 until they are rendered. **/

			for (;i<j;i++)
			{
				if (iswhitespace(txt[i]))
				{
					if (linebegan)
					{
						app->pagebuf[page->length] = ' ';
						page->length++;
					}
				}
				else
				{
					linebegan = true;
					app->pagebuf[page->length] = txt[i];
					page->length++;
				}
			}
			pdata->pen.x += advance;
		}
	}
}  /* End char_hndl */

void end_hndl(void *data, const char *el)
{
	page_t *page = &(app->pages[app->pagecurrent]);
	parsedata_t *p = (parsedata_t *)data;
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
	    || !stricmp(el,"ul")
	)
	{
		if(linebegan) {
			linebegan = false;
			app->pagebuf[page->length] = '\n';
			page->length++;
			p->pen.x = MARGINLEFT;
			p->pen.y += app->ts->GetHeight() + LINESPACING;
			if (	!stricmp(el,"p")
				|| !strcmp(el,"h1")
				|| !strcmp(el,"h2")
				|| !strcmp(el,"h3")
				|| !strcmp(el,"h4")
				|| !strcmp(el,"h5")
				|| !strcmp(el,"h6")
				|| !strcmp(el,"hr")
				|| !strcmp(el,"pre")
			)
			{
				app->pagebuf[page->length] = '\n';
				page->length++;
				p->pen.x = MARGINLEFT;
				p->pen.y += app->ts->GetHeight() + LINESPACING;
			}
			if (p->pen.y > (PAGE_HEIGHT-MARGINBOTTOM))
			{
				if (app->fb == app->screen1)
				{
					app->fb = app->screen0;
					if (!page->buf)
						page->buf = (u8*)new u8[page->length];
					strncpy((char*)page->buf,(char *)app->pagebuf,page->length);
					page++;
					app->page_init(page);
					app->pagecurrent++;
					app->pagecount++;
					if(app->pagecount == MAXPAGES) return;
				}
				else
				{
					app->fb = app->screen1;
				}
				p->pen.x = MARGINLEFT;
				p->pen.y = MARGINTOP + app->ts->GetHeight();
			}
		}
	}

	if (!stricmp(el,"body"))
	{
		if (!page->buf)
		{
			page->buf = new u8[page->length];
			if (!page->buf)
				app->ts->PrintStatusMessage("[memory error storing page]\n");
		}
		strncpy((char*)page->buf,(char*)app->pagebuf,page->length);
		app->parse_pop(p);
	}

	app->parse_pop(p);


}  /* End of end_hndl */

void proc_hndl(void *data, const char *target, const char *pidata)
{
}  /* End proc_hndl */

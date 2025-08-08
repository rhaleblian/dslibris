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

#include "main.h"

#include <sys/param.h>
#include "dirent.h"
#include "fat.h"
#include "nds.h"

#include "app.h"
#include "book.h"
#include "button.h"
#include "text.h"
#include "expat.h"
#include "parse.h"

App *app;
char msg[256];
int ft_main(int argc, char **argv);


//! \param vblanks blanking intervals to wait, -1 for forever, default = -1
int halt(int vblanks) {
	int timer = vblanks;
	while(pmMainLoop()) {
		swiWaitForVBlank();
		if (timer == 0) break;
		else if (timer > 0) timer--;
	}
	return 1;
}

//! \param vblanks blanking intervals to wait, -1 for forever, default = -1
int halt(const char *msg, int vblanks) {
	consoleDemoInit();
	printf(msg);
	return halt(vblanks);
}

int main(void)
{
	defaultExceptionHandler();
	// consoleDemoInit();
	// consoleDebugInit(DebugDevice_NOCASH);

	if (!fatInitDefault())
		halt("[FAIL] filesystem\n");

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
	// FIXME this will never run
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
				
				app->invert = atoi(attr[i+1]);
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
			else if(!strcmp(attr[i],"bolditalic"))
				app->ts->SetFontFile((char *)attr[i+1], TEXT_STYLE_BOLDITALIC);
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
	return 0;
}

void default_hndl(void *data, const XML_Char *s, int len)
{
	// Handles HTML entities in body text.

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

int getSize(u8 *source, u16 *dest, u32 arg) {
       return *(u32*)source;
}

u8 readByte(u8 *source) { return *source; }

void drawstack(u16 *screen) {
       TDecompressionStream decomp = {getSize, NULL, readByte};
       swiDecompressLZSSVram((void*)splashBitmap, screen, 0, &decomp);
}

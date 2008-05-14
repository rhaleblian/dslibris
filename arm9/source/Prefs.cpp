#include <stdio.h>
#include "nds.h"
#include "main.h"
#include "Prefs.h"
#include "Book.h"
#include <vector>

Prefs::Prefs(App *parent) { app = parent; }
Prefs::~Prefs() {}
	
bool Prefs::Read(XML_Parser p)
{
	FILE *fp = fopen(PREFSPATH,"r");
	if (!fp) return false;

	XML_ParserReset(p, NULL);
	XML_SetStartElementHandler(p, prefs_start_hndl);
	XML_SetUserData(p, (void *)app->books);
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

bool Prefs::Write(void)
{
	FILE* fp = fopen(PREFSPATH,"w");
	if(!fp) return false;
	
	fprintf(fp, "<dslibris>\n");
	fprintf(fp, "\t<screen brightness=\"%d\" invert=\"%d\" />\n",
		app->brightness,
		app->ts->GetInvert());
	fprintf(fp,	"\t<margin top=\"%d\" left=\"%d\" bottom=\"%d\" right=\"%d\" />\n",	
			app->margintop, app->marginleft,
			app->marginbottom, app->marginright);
 	fprintf(fp, "\t<font path=\"%s\" file=\"%s\" size=\"%d\" />\n",
 		app->fontdir.c_str(),
		app->ts->GetFontFile().c_str(),
		app->ts->GetPixelSize());
 	fprintf(fp, "\t<paragraph indent=\"%d\" spacing=\"%d\" />\n",
			app->paraindent,
			app->paraspacing);
	/* TODO save pagination data with current book to cache it to disk.
	   store timestamp too in order to invalidate caches.
	vector<u16> pageindices;
	for(u16 i=0;i<app->pagecount;i++)
	{
		
	}
	*/
 	char* pathname = app->books[app->bookcurrent].GetFullPathName();
    fprintf(fp, "\t<books path=\"%s\" reopen=\"%s\">\n",
    		app->bookdir.c_str(),
            (app->reopen && app->bookcurrent >= 0) ? pathname : "");
    delete[] pathname;
    for (u8 i = 0; i < app->bookcount; i++) {
        Book* book = app->books + i;
        pathname = book->GetFullPathName();
        fprintf(fp, "\t\t<book file=\"%s\" page=\"%d\">\n",
                pathname, book->GetPosition() + 1);
        delete[] pathname;
		std::list<u16>* bookmarks = book->GetBookmarks();
        for (std::list<u16>::iterator j = bookmarks->begin(); j != bookmarks->end(); j++) {
            fprintf(fp, "\t\t\t<bookmark page=\"%d\" />\n",
                    *j + 1);
        }

        fprintf(fp, "\t\t</book>\n");
    }

    fprintf(fp, "\t</books>\n");
	
	fprintf(fp, "</dslibris>\n");
	fprintf(fp, "\n");
	fclose(fp);

	return true;
}

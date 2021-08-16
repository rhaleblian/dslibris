#include <stdio.h>
#include <vector>
#include "sys/stat.h"
#include "sys/time.h"
#include "nds.h"
#include "main.h"
#include "app.h"
#include "prefs.h"
#include "book.h"

#define PARSEBUFSIZE 1024*64

Prefs::Prefs() {
	Init();
}
Prefs::Prefs(App *parent) { Init(); app = parent; }
Prefs::~Prefs() {}

//! \return 0: success, 255: file open failure, 254: no bytes read, 253: parse failure.
int Prefs::Read()
{
	int err = 0;
	parsedata_t pdata;
	app->parse_init(&pdata);
	pdata.prefs = this;
		
	FILE *fp = fopen(PREFSPATH,"r");
	if (!fp) { err = 255; return err; }

	XML_Parser p = XML_ParserCreate(NULL);
	if(!p) { fclose(fp); err = 254; return err; }
	XML_SetUnknownEncodingHandler(p,unknown_hndl,NULL);
	XML_SetStartElementHandler(p, prefs_start_hndl);
	XML_SetEndElementHandler(p, prefs_end_hndl);
	XML_SetUserData(p, (void *)&pdata);
	while (true)
	{
	 	void *buff = XML_GetBuffer(p, PARSEBUFSIZE);
	 	int bytes_read = fread(buff, sizeof(char), PARSEBUFSIZE, fp);
		if(bytes_read < 0) { err = 254; break; }
		enum XML_Status status = 
			XML_ParseBuffer(p, bytes_read, bytes_read == 0);
		if(status == XML_STATUS_ERROR) { 
			app->parse_error(p);
			err = 253;
			break;
		}
		if (bytes_read == 0) break;
	}
	XML_ParserFree(p);
	fclose(fp);
	return err;

	struct stat st;
	stat(PREFSPATH,&st);
	struct timeval time;
	gettimeofday(&time,NULL);
	char msg[64];
	sprintf(msg,"info : file timestamp %lld",st.st_mtime);
	app->Log(msg);
	sprintf(msg,"info : current time %lld",time.tv_sec);
	app->Log(msg);
}

//! \return Error code, 0: success.
int Prefs::Write()
{
	int err = 0;
	FILE* fp = fopen(PREFSPATH,"w");
	if(!fp) return 255;
	
	fprintf(fp, "<dslibris>\n");
	if(swapshoulder)
		fprintf(fp, "<option swapshoulder=\"%d\" />\n",swapshoulder);		
	fprintf(fp, "\t<screen brightness=\"%d\" invert=\"%d\" flip=\"%d\" />\n",
		app->brightness,
		app->ts->GetInvert(),
		app->orientation);
	fprintf(fp,	"\t<margin top=\"%d\" left=\"%d\" bottom=\"%d\" right=\"%d\" />\n",	
			app->ts->margin.top, app->ts->margin.left,
			app->ts->margin.bottom, app->ts->margin.right);
 	fprintf(fp, "\t<font size=\"%d\" normal=\"%s\" bold=\"%s\" italic=\"%s\" />\n",
		app->ts->GetPixelSize(),
		app->ts->GetFontFile(TEXT_STYLE_REGULAR).c_str(),
		app->ts->GetFontFile(TEXT_STYLE_BOLD).c_str(),
		app->ts->GetFontFile(TEXT_STYLE_ITALIC).c_str());
 	fprintf(fp, "\t<paragraph indent=\"%d\" spacing=\"%d\" />\n",
			app->paraindent,
			app->paraspacing);
	/* TODO save pagination data with current book to cache it to disk.
	   store timestamp too in order to invalidate caches.
	vector<u16> pageindices;
	for(u16 i=0;i<app->pagecount;i++) {}
	*/
    fprintf(fp, "\t<books path=\"%s\" reopen=\"%d\">\n",
    		app->bookdir.c_str(),
    		app->reopen);
    
    for (u8 i = 0; i < app->bookcount; i++) {
        Book* book = app->books[i];
        fprintf(fp, "\t\t<book file=\"%s\" page=\"%d\"",
                book->GetFileName(), book->GetPosition() + 1);
		if(app->bookcurrent == app->books[i]) fprintf(fp," current=\"1\"");
		fprintf(fp,">\n");
		std::list<u16>* bookmarks = book->GetBookmarks();
        for (std::list<u16>::iterator j = bookmarks->begin(); j != bookmarks->end(); j++) {
            fprintf(fp, "\t\t\t<bookmark page=\"%d\" word=\"%d\" />\n",
                    *j + 1,0);
        }

        fprintf(fp, "\t\t</book>\n");
    }

    fprintf(fp, "\t</books>\n");
	
	fprintf(fp, "</dslibris>\n");
	fprintf(fp, "\n");
	fclose(fp);

	return err;
}

void Prefs::Init(){
	modtime = 0;  // fill this in with gettimeofday()
	swapshoulder = FALSE;
}

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

Prefs::Prefs(App *_app) { app = _app; Init(); }
Prefs::~Prefs() {}

//! \return 0: success, 255: file open failure, 254: no bytes read, 253: parse failure.
int Prefs::Read()
{
	int err = 0;

	FILE *fp = fopen(PREFSPATH,"r");
	if (!fp)
	{ err = 255; return err; }

	parsedata_t pdata;
	app->parse_init(&pdata);
	pdata.prefs = this;

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
			err = XML_GetErrorCode(p);
			break;
		}
		if (bytes_read == 0) break;
	}
	XML_ParserFree(p);
	fclose(fp);
	return err;
}

//! Write settings to PREFSPATH.
//! \return Error code.
int Prefs::Write()
{
	if (app->melonds) return 0;

	int err = 0;
	int invert = 0;

	if (app) invert = app->ts->GetInvert();

	FILE* fp = fopen(PREFSPATH, "w");
	if(!fp) return 255;
	
	fprintf(fp, "<dslibris format=\"2\">\n");
	if(swapshoulder)
		fprintf(fp, "<option swapshoulder=\"%d\" />\n",swapshoulder);
	fprintf(fp, "\t<screen invert=\"%d\" flip=\"%d\" />\n",
		//TODO FIX THIS
		invert,
		app->orientation
		);
	fprintf(fp,	"\t<margin top=\"%d\" left=\"%d\" bottom=\"%d\" right=\"%d\" />\n",	
			app->ts->margin.top, app->ts->margin.left,
			app->ts->margin.bottom, app->ts->margin.right);
 	fprintf(fp, "\t<font size=\"%d\" normal=\"%s\" bold=\"%s\" italic=\"%s\" bolditalic=\"%s\" />\n",
		app->ts->GetPixelSize(),
		app->ts->GetFontFile(TEXT_STYLE_REGULAR).c_str(),
		app->ts->GetFontFile(TEXT_STYLE_BOLD).c_str(),
		app->ts->GetFontFile(TEXT_STYLE_ITALIC).c_str(),
		app->ts->GetFontFile(TEXT_STYLE_BOLDITALIC).c_str()
	);
 	fprintf(fp, "\t<paragraph indent=\"%d\" spacing=\"%d\" />\n",
			app->paraindent,
			app->paraspacing);
	fprintf(fp, "\t<books reopen=\"%d\">\n",
			app->reopen);
    
	for (u8 i = 0; i < app->bookcount; i++) {
		Book* book = app->books[i];
		fprintf(fp, "\t\t<book file=\"%s\" page=\"%d\"",
		book->GetFileName(), book->GetPosition() + 1);
		if(app->bookcurrent == app->books[i]) fprintf(fp," current=\"1\"");
		fprintf(fp,">\n");
		std::list<u16>* bookmarks = book->GetBookmarks();
		for (std::list<u16>::iterator j = bookmarks->begin();
			j != bookmarks->end();
			j++) {
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

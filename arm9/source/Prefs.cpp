#include <stdio.h>
#include "nds.h"
#include "main.h"
#include "Prefs.h"
#include "Book.h"

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
	FILE* fp = fopen(PREFSPATH,"w+");
	if(!fp) return false;
	
	fprintf(fp, "<dslibris>\n");
	fprintf(fp, "\t<screen brightness=\"%d\" invert=\"%d\">\n",
		app->brightness, app->ts->GetInvert());
	fprintf(fp, "\t<font size=\"%d\" />\n", app->ts->GetPixelSize());
	fprintf(fp, "\t<book file=\"%s\" />\n", app->books[app->bookcurrent].GetFileName());
	for(u8 i=0;i<app->bookcount; i++)
	{
		fprintf(fp, "\t<bookmark file=\"%s\" page=\"%d\" />\n",
	        (app->books)[i].GetFileName(), app->books[i].GetPosition()+1);
	}
	fprintf(fp, "</dslibris>\n");
	fclose(fp);

	return true;
}

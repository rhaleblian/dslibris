#include "Book.h"
#include "main.h"
#include "parse.h"
#include "App.h"
#include <tidy.h>
#include <buffio.h>
#include <stdio.h>
#include <errno.h>

extern App *app;

Book::Book()
{
	filename.clear();
	title.clear();
	author.clear();
	position = 0;
}

void Book::SetFilename(const char *name)
{
	filename = std::string(name);
}

void Book::SetTitle(const char *name)
{
	title = std::string(name);
}

const char* Book::GetTitle()
{
	return title.c_str();
}

const char* Book::GetFilename()
{
	return filename.c_str();
}

s16 Book::GetPosition()
{
	return position;
}

void Book::SetPosition(s16 pos)
{
	position = pos;
}

void Book::Index(char *filebuf)
{
	char path[128] = BOOKPATH;
	strcat(path,"/");
	strcat(path,filename.c_str());
	FILE *fp = fopen(path,"r");
	if (fp)
	{
		XML_Parser p = XML_ParserCreate(NULL);
		parsedata_t parsedata;
		app->parse_init(&parsedata);
		parsedata.book = this;
		XML_SetUserData(p, &parsedata);
		XML_SetElementHandler(p, start_hndl, end_hndl);
		XML_SetCharacterDataHandler(p, title_hndl);
		XML_SetProcessingInstructionHandler(p, proc_hndl);
		while (true)
		{
			int bytes_read = fread(filebuf, 1, BUFSIZE, fp);
			if (XML_Parse(p, filebuf, bytes_read,bytes_read == 0)) break;
			if (!bytes_read) break;
		}
		XML_ParserFree(p);
		fclose(fp);
	}
}

void Book::IndexHTML(char *filebuf)
{
	TidyBuffer output = {0};
	int rc = -1;
	Bool ok;

	char path[128] = BOOKPATH;
	strcat(path,"/");
	strcat(path,filename.c_str());
	FILE *fp = fopen(path,"r");
	if (fp)
	{	
		fread(filebuf, 1, BUFSIZE, fp);		

		TidyDoc tdoc = tidyCreate();
		ok = tidyOptSetBool(tdoc, TidyXhtmlOut, yes);		
		if(ok)
			rc = tidyParseString(tdoc,filebuf);

		TidyNode head = tidyGetHead(tdoc);
		TidyNode child = tidyGetChild(head);
		while(child) 
		{
			if(tidyNodeIsTITLE(child))
			{
				tidyNodeGetText(tdoc,tidyGetChild(child),&output);
				SetTitle((char*)output.bp);
				break;
			}
			child = tidyGetNext(child);
		}

/*
		if(rc >= 0)
			rc = tidyCleanAndRepair(tdoc);
		if(rc >= 0)
			rc = (tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1);
		if(rc >= 0)
			rc = tidySaveBuffer(tdoc, &output);
*/

		tidyBufFree(&output);
		tidyRelease(tdoc);
		fclose(fp);
	}
}

u8 Book::Parse(char *filebuf)
{
	char path[128] = BOOKPATH;
	strcat(path,"/");
	strcat(path,filename.c_str());
	FILE *fp = fopen(path,"r");
	if (!fp)
	{
		return(1);
	}

	XML_Parser p = XML_ParserCreate(NULL);
	parsedata_t parsedata;
	XML_ParserReset(p,NULL);
	app->parse_init(&parsedata);
	XML_SetUserData(p, &parsedata);
	XML_SetDefaultHandler(p, default_hndl);
	XML_SetElementHandler(p, start_hndl, end_hndl);
	XML_SetCharacterDataHandler(p, char_hndl);
	XML_SetProcessingInstructionHandler(p, proc_hndl);

	enum XML_Status status;
	while (true)
	{
		int bytes_read = fread(filebuf, 1, BUFSIZE, fp);
		status = XML_Parse(p, filebuf, bytes_read, (bytes_read == 0));
		if (status == XML_STATUS_ERROR)
		{
			app->parse_printerror(p);
			break;
		}
		if (bytes_read == 0) break;
	}

	XML_ParserFree(p);
	fclose(fp);
	return(0);
}

u8 Book::ParseHTML(char *filebuf)
{
	TidyBuffer output = {0};
	int rc = -1;
	filebuf = (char*)malloc(1024*sizeof(char));

	char path[128] = BOOKPATH;
	strcat(path,"/");
	strcat(path,filename.c_str());
	FILE *fp = fopen(path,"r");
	if (!fp) return 1;

	fread(filebuf, 1, BUFSIZE, fp);		

	TidyDoc tdoc = tidyCreate();
	rc = tidyParseString(tdoc,filebuf);
	TidyNode body = tidyGetHead(tdoc);
	TidyNode child = tidyGetChild(body);
	while(child)
	{
		if(tidyNodeGetType(child) == TidyNode_Text)
		{
			tidyNodeGetText(tdoc,child,&output);
		}
		child = tidyGetNext(child);
	}

	strncat((char*)app->pages[0].buf, (char*)output.bp, 512);

	tidyBufFree(&output);
	tidyRelease(tdoc);
	fclose(fp);
	return 0;
}

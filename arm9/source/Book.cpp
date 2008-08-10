#include "Book.h"
#include "main.h"
#include "parse.h"
#include "App.h"
#include <tidy.h>
#include <buffio.h>
#include <stdio.h>
#include <errno.h>

extern App *app;
extern bool parseFontBold;
extern bool parseFontItalic;

Book::Book()
{
	foldername.clear();
	filename.clear();
	title.clear();
	author.clear();
	position = 0;
}

void Book::SetFolderName(const char *name)
{
	foldername.clear();
	foldername = name;
}

void Book::SetFolderName(std::string &name)
{
	foldername = name;
}

void Book::SetFileName(const char *name)
{
	filename.clear();
	filename = name;
}

void Book::SetTitle(const char *name)
{
	title.clear();
	title = name;
}

std::list<u16>* Book::GetBookmarks()
{
    return &bookmarks;
}

const char* Book::GetTitle()
{
	return title.c_str();
}

const char* Book::GetFileName()
{
	return filename.c_str();
}

const char* Book::GetFolderName()
{
	return foldername.c_str();
}

u16 Book::GetPosition()
{
	return position;
}

void Book::SetPosition(s16 pos)
{
	position = pos;
}

u8 Book::Index(char *filebuf)
{
	char path[128];
	if(foldername.length()) {
		strcpy(path,foldername.c_str());
		strcat(path,"/");
	} else strcpy(path,"");
	strcat(path,filename.c_str());
	
	FILE *fp = fopen(path,"r");
	if(!fp) return(255);
	XML_Parser p = XML_ParserCreate(NULL);
	if(!p) return(254);
	parsedata_t parsedata;
	app->parse_init(&parsedata);
	parsedata.book = this;
	XML_SetUserData(p, &parsedata);
	XML_SetElementHandler(p, title_start_hndl, title_end_hndl);
	XML_SetCharacterDataHandler(p, title_char_hndl);
	XML_SetProcessingInstructionHandler(p, proc_hndl);
	while (true)
	{
		int bytes_read = fread(filebuf, 1, BUFSIZE, fp);
		if (XML_Parse(p, filebuf, bytes_read,bytes_read == 0)) break;
		if (!bytes_read) break;
	}
	XML_ParserFree(p);
	fclose(fp);
	return(0);
}


int Book::ParseHTML(char *input)
{
	TidyBuffer output;
	TidyBuffer errbuf;
	int rc = -1;
	Bool ok;

	char *path = GetFullPathName();
//	FILE *fp = fopen(path,"r");
//	FILE *op = fopen("tmp.xhtml","w");

	TidyDoc tdoc = tidyCreate();
//	tidyBufInit(&output);
//	tidyBufInit(&errbuf);
	ok = tidyOptSetBool( tdoc, TidyXhtmlOut, yes );  // Convert to XHTML
	if ( ok )
		//rc = tidySetErrorBuffer( tdoc, &errbuf );    // Capture diagnostics
		tidySetErrorFile( tdoc , "dslibris.log" );
/*
	while(fread(input,1,BUFSIZE,fp))
	{
		if ( rc >= 0 )
			rc = tidyParseString( tdoc, input );     // Parse the input
		if ( rc >= 0 )
			rc = tidyCleanAndRepair( tdoc );         // Tidy it up!
	}
*/

	rc = tidyParseFile( tdoc, path );
	if ( rc >= 0 )
		rc = tidyCleanAndRepair( tdoc );         // Tidy it up!

	if ( rc >= 0 )
		rc = tidyRunDiagnostics( tdoc );               // Kvetch
	if ( rc > 1 )                                  // If error, force output.
		rc = ( tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1 );
	rc = tidyOptSetBool( tdoc, TidyNumEntities, yes );
	if ( rc > 1 )
		rc = tidySetCharEncoding ( tdoc, "utf8" );

//	if ( rc >= 0 )
//		rc = tidySaveBuffer( tdoc, &output );          // Pretty Print
/*
	if ( rc >= 0 )
	{
		if ( rc > 0 )
			fprintf( op, "%s", output.bp );
	}
	else
		printf( "A severe error (\%d) occurred.\\n", rc );
*/

	tidySaveFile( tdoc, "/tmp.xhtml" );

//	fclose(fp);
//	fclose(op);

//	tidyBufFree( &output );
//	tidyBufFree( &errbuf );
	tidyRelease( tdoc );

	char file[256];
	char folder[256];
	strcpy(file,GetFileName());
	strcpy(folder,GetFolderName());
	SetFileName("tmp.xhtml");
	SetFolderName("/");	
	rc = Parse(input);
	SetFileName(file);
	SetFolderName(folder);

	return rc;
}


u8 Book::Parse(char *filebuf)
{
	u8 rc = 0;
	char* path = GetFullPathName();
	FILE *fp = fopen(path,"r");
	if (!fp)
	{
		rc = 255;
		return(rc);
	}
	delete[] path;

	parseFontBold = false;
	parseFontItalic = false;
	
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
			rc = 254;
			break;
		}
		if (bytes_read == 0) break;
	}

	XML_ParserFree(p);
	fclose(fp);
	return(rc);
}

char* Book::GetFullPathName()
{
	char* path = new char[MAXPATHLEN];
	strcpy(path, foldername.c_str());
	strcat(path, filename.c_str());
	
	return (char*)path;
}

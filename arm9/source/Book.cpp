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

u8 Book::Parse(char *filebuf)
{
	u8 rc = 0;
	const char* path = GetFullPathName();
	FILE *fp = fopen(path,"r");
	if (!fp)
	{
		rc = 255;
		return(rc);
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
			rc = 254;
			break;
		}
		if (bytes_read == 0) break;
	}

	XML_ParserFree(p);
	fclose(fp);
	return(rc);
}

const char* Book::GetFullPathName()
{
	char path[MAXPATHLEN];
	strcpy(path, foldername.c_str());
	strcat(path, filename.c_str());
	
	return (const char*)path;
}

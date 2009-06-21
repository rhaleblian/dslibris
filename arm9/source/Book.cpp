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
	pages.clear();
	position = 0;
}

Book::~Book()
{
	Close();
}

void Book::SetFolderName(const char *name)
{
	foldername.clear();
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

void Book::SetFolderName(std::string &name)
{
	foldername = name;
}

std::list<u16>* Book::GetBookmarks()
{
    return &bookmarks;
}
	
Page* Book::GetPage()
{
	return pages[position];
}

Page* Book::GetPage(int index)
{
	return pages[index];
}

u16 Book::GetPageCount()
{
	return pages.size();
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

int Book::GetPosition()
{
	return position;
}

void Book::SetPage(u16 index)
{
	position = index;
}

void Book::SetPosition(int pos)
{
	position = pos;
}

Page* Book::AppendPage()
{
	Page *page = new Page(this);
	pages.push_back(page);
	return page;
}

Page* Book::AdvancePage()
{
	if(position < (int)pages.size()) position++;
	return GetPage();
}

Page* Book::RetreatPage()
{
	if(position > 0) position--;
	return GetPage();
}

u8 Book::Open() {
	int err = Parse(true);
	if (err) return err;
	if(position > (int)pages.size()) position = pages.size()-1;
	return 0;
}

u8 Book::Index()
{
	return Parse(false);
}

u8 Book::Parse(bool fulltext)
{
	//! Parse full text (true) or titles only (false).
	//! Expat callback handlers do the heavy work.
	u8 rc = 0;
	
	char *filebuf = new char[BUFSIZE];
	if(!filebuf)
	{
		rc = 1;
		return(rc);
	}
	
	char path[MAXPATHLEN];
	sprintf(path,"%s%s",GetFolderName(),GetFileName());
	FILE *fp = fopen(path,"r");
	if (!fp)
	{
		free(filebuf);
		rc = 255;
		return(rc);
	}
	
	parsedata_t parsedata;
	app->parse_init(&parsedata);	
	parsedata.book = this;
	
	XML_Parser p = XML_ParserCreate(NULL);
	if(!p)
	{
		free(filebuf);
		fclose(fp);
		rc = 253;
		return rc;
	}
	XML_ParserReset(p,NULL);
	XML_SetUserData(p, &parsedata);
	XML_SetDefaultHandler(p, default_hndl);
	XML_SetProcessingInstructionHandler(p, proc_hndl);
	if(fulltext)
	{
		XML_SetElementHandler(p, start_hndl, end_hndl);
		XML_SetCharacterDataHandler(p, char_hndl);
	}
	else
	{
		XML_SetElementHandler(p, title_start_hndl, title_end_hndl);
		XML_SetCharacterDataHandler(p, title_char_hndl);
	}
	
	enum XML_Status status;
	while (true)
	{
		int bytes_read = fread(filebuf, 1, BUFSIZE, fp);
		status = XML_Parse(p, filebuf, bytes_read, (bytes_read == 0));
		if (status == XML_STATUS_ERROR)
		{
			app->parse_error(p);
			rc = 254;
			break;
		}
		if (bytes_read == 0) break;
	}

	XML_ParserFree(p);
	fclose(fp);
	free(filebuf);
	return(rc);
}

void Book::Close()
{
	vector<Page*>::iterator it;
	for (it = pages.begin(); it != pages.end(); it++)
	{
		if(*it) delete *it;
	}
	pages.erase(pages.begin(), pages.end());
}

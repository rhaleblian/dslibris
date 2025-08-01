#include "book.h"

#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include "main.h"
#include "parse.h"
#include "epub.h"
#include "app.h"

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
	format = FORMAT_UNDEF;
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

void Book::SetAuthor(std::string &name)
{
	author.clear();
	author = name;
}

void Book::SetFolderName(std::string &name)
{
	foldername = name;
}

std::list<u16>* Book::GetBookmarks()
{
    return &bookmarks;
}

int Book::GetNextBookmark()
{
	//! nyi
	return -1337;
}

int GotoNextBookmarkedPage()
{
	//! nyi
	return -1337;

}

int Book::GetPreviousBookmark()
{
	//! Return previous page index,
	//! relative to current one.
	//! nyi
	return -1337;
}

int GotoPreviousBookmarkedPage()
{
	return -1337;
}

int Book::GetPosition(int offset)
{
  //! For the character offset, get the page.
  return -1337;
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

void Book::Cache()
{
	FILE *fp = fopen("/cache.dat", "w");
	if (!fp) return;

	int buflen = 0;
	int pagecount = GetPageCount();
	fprintf(fp, "%d\n", pagecount);
	for(int i=0; i<pagecount; i++)	{
		buflen += GetPage(i)->GetLength();
		fprintf(fp, "%d\n", buflen);
		GetPage(i)->Cache(fp);
	}
	fclose(fp);
}

u8 Book::Open() {
	int err = 0;

	FILE *fp = fopen("/cache.dat", "r");
	if (fp && app->cache) {
		// Restore from cache.
		fclose(fp);
		Restore();
	} else {
		std::string path;
		path.append(GetFolderName());
		path.append("/");
		path.append(GetFileName());
		app->ts->SetStyle(TEXT_STYLE_REGULAR);
		err = epub(this,path,false);
		// TODO toss this.
		if (app->cache) Cache();
		fclose(fp);
	}
	if (!err)
		if(position > (int)pages.size()) position = pages.size()-1;

	return (u8)err;
}

u8 Book::Index()
{
	std::string path;
	path.append(GetFolderName());
	path.append("/");
	path.append(GetFileName());
	int err = epub(this,path,true);
	return err;
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
		delete [] filebuf;
		rc = 255;
		return(rc);
	}
	
	parsedata_t parsedata;
	app->parse_init(&parsedata);
	parsedata.cachefile = fopen("/cache.dat", "w");
	parsedata.book = this;
	
	XML_Parser p = XML_ParserCreate(NULL);
	if(!p)
	{
		delete [] filebuf;
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
		if (parsedata.status) break; // non-fulltext parsing signals it is done.
		if (bytes_read == 0) break; // assume our buffer ran out.
	}

	XML_ParserFree(p);
	fclose(fp);
	delete [] filebuf;

	return(rc);
}

// TODO toss this
void Book::Restore()
{
	FILE *fp = fopen("/cache.dat", "r");
	if( !fp ) return;

	int len, pagecount;
	u8 buf[BUFSIZE];

	fscanf(fp, "%d\n", &pagecount);
	for (int i=0; i<pagecount-1; i++) {
		fscanf(fp, "%d\n", &len);
		fread(buf, sizeof(char), len, fp);
		GetPage(i)->SetBuffer(buf, len);
	}
	fclose(fp);
}

void Book::Close()
{
	std::vector<Page*>::iterator it = pages.begin();
	while (it != pages.end()) {
	    delete *it;
	    *it = nullptr;
	    ++it;
	}
	pages.clear();	
}

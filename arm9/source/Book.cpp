#include "Book.h"
#include "main.h"
#include "parse.h"
#include "App.h"

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
	FILE *fp = fopen(filename.c_str(),"r");
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

u8 Book::Parse(char *filebuf)
{
	FILE *fp = fopen(filename.c_str(),"r");
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

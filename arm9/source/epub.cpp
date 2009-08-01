#include <nds.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include "parse.h"
#include "expat.h"
#include "zlib.h"
#include "unzip.h"
#include "Book.h"
#include "log.h"

typedef enum { PARSE_CONTAINER, PARSE_ROOTFILE, PARSE_CONTENT } epub_parse_t;
typedef struct {
	epub_parse_t type;
	std::string ctx;
	std::string rootfile;
	std::vector<std::string*> manifest;
	Book *book;
} epub_data_t;


void epub_data_init(epub_data_t *d)
{
	d->type = PARSE_CONTAINER;
	d->ctx = "";
	d->rootfile = "";
	d->book = NULL;
}

void epub_data_delete(epub_data_t *d)
{
	std::vector<std::string*>::iterator it;
	for(it=d->manifest.begin(); it!=d->manifest.end();it++)
		delete (*it);
	d->manifest.clear();
}

void epub_container_start(void *data, const char *el, const char **attr)
{
	epub_data_t *d = (epub_data_t*)data;
	if(!stricmp(el,"rootfile"))
		for(int i=0;attr[i];i+=2)
			if(!stricmp(attr[i],"full-path"))
				d->rootfile = attr[i+1];
}


void epub_rootfile_start(void *data, const char *el, const char **attr)
{
	epub_data_t *d = (epub_data_t*)data;
	if(!stricmp(el,"manifest"))
		d->ctx = "manifest";
	else if(!stricmp(el,"item") && d->ctx == "manifest")
		for(int i=0;attr[i];i+=2)
			if(!stricmp(attr[i],"href"))
				d->manifest.push_back(new std::string(attr[i+1]));
}


int epub_parse_currentfile(unzFile uf, epub_data_t *epd)
{
	int rc = 0;
	parsedata_t pd;
	char *filebuf = new char[BUFSIZE];
	XML_Parser p = XML_ParserCreate(NULL);
	if(epd->type == PARSE_CONTAINER) {
		XML_SetUserData(p, epd);
		XML_SetElementHandler(p, epub_container_start, NULL);
	}
	else if(epd->type == PARSE_ROOTFILE) {
		XML_SetUserData(p, epd);
		XML_SetElementHandler(p, epub_rootfile_start, NULL);
	}
	else {
		parse_init(&pd);
		pd.book = epd->book;
		XML_SetUserData(p, &pd);
		XML_SetElementHandler(p, start_hndl, end_hndl);
		XML_SetCharacterDataHandler(p, char_hndl);
		XML_SetDefaultHandler(p, default_hndl);
		XML_SetProcessingInstructionHandler(p, proc_hndl);
	}
	size_t len, len_total=0;
	char msg[64];
	enum XML_Status status;
	do {
		len = unzReadCurrentFile(uf,filebuf,BUFSIZE);
		status = XML_Parse(p, filebuf, len, len == 0);
		if (status == XML_STATUS_ERROR) {
			sprintf(msg,"error: expat %d\n", status); Log(msg);
			rc = status;
			break;
		}
		len_total += len;
		sprintf(msg,"progr: %d bytes.\n",len); Log(msg);
	} while (!len);
	XML_ParserFree(p);
	delete filebuf;
	return(rc);
}


int epub(Book *book, std::string name)
{
	int rc = 0;
	static epub_data_t parsedata;
	
	unzFile uf = unzOpen(name.c_str());
	rc = unzLocateFile(uf,"META-INF/container.xml",0);
	if(rc == UNZ_OK)
	{
		rc = unzOpenCurrentFile(uf);
		epub_data_init(&parsedata);
		parsedata.type = PARSE_CONTAINER;
		rc = epub_parse_currentfile(uf, &parsedata);
		rc = unzCloseCurrentFile(uf);
	}
	
	Log("info : rootfile "); Log(parsedata.rootfile); Log("\n");
	
	// Extract any leading path for the rootfile.
	// The manifest in the rootfile will list filenames
	// relative to the rootfile location.
	std::string folder = "";
	size_t pos = parsedata.rootfile.find_last_of("/",parsedata.rootfile.length());
	if(pos < parsedata.rootfile.length()) {
		folder = parsedata.rootfile.substr(0,pos);
	}

	rc = unzLocateFile(uf,parsedata.rootfile.c_str(),0);
	if(rc == UNZ_OK)
	{
		rc = unzOpenCurrentFile(uf);
		epub_data_init(&parsedata);
		parsedata.type = PARSE_ROOTFILE;
		epub_parse_currentfile(uf, &parsedata);
		rc = unzCloseCurrentFile(uf);
	}

	// Read all the XHTML listed, in sequence.
	parsedata.ctx = "";
	parsedata.book = book;
	parsedata.type = PARSE_CONTENT;
	vector<std::string*>::iterator it;
	for(it=parsedata.manifest.begin();it!=parsedata.manifest.end();it++)
	{
		size_t pos = (*it)->find_last_of('.');
		if (pos < (*it)->length())
		{
			std::string ext = (*it)->substr(pos);
			if(ext.find("htm") < ext.length())
			{
				std::string path = folder;
				if(path.length()) path += "/";
				path += (*it)->c_str();
			    Log("info : content "); Log(path); Log("\n");
				rc = unzLocateFile(uf,path.c_str(),0);
				if(rc == UNZ_OK)
				{
					rc = unzOpenCurrentFile(uf);
					epub_parse_currentfile(uf, &parsedata);
					rc = unzCloseCurrentFile(uf);
				}
			}
		}
	}
	unzClose(uf);
	epub_data_delete(&parsedata);
	return rc;
}

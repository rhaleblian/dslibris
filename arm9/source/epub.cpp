#include "epub.h"
#include <nds.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include "parse.h"
#include "expat.h"
#include "zlib.h"
#include "unzip.h"
#include "log.h"

void epub_data_init(epub_data_t *d)
{
	d->type = PARSE_CONTAINER;
	d->ctx.clear();
	d->rootfile = "";
	d->title = "";
	d->creator = "";
	d->metadataonly = false;
	d->book = NULL;
}

void epub_data_delete(epub_data_t *d)
{
	std::vector<std::string*>::iterator it;
	for(it=d->manifest.begin(); it!=d->manifest.end();it++)
		delete (*it);
	d->manifest.clear();
	while(d->ctx.back()) d->ctx.pop_back();
}

void epub_container_start(void *data, const char *el, const char **attr)
{
	epub_data_t *d = (epub_data_t*)data;
	if(!stricmp(el,"rootfile"))
		for(int i=0;attr[i];i+=2)
			if(!stricmp(attr[i],"full-path"))
				d->rootfile = attr[i+1];
}

void epub_rootfile_start(void *data, const char *el, const char **attr) {
	epub_data_t *d = (epub_data_t*)data;
	d->ctx.push_back(new std::string(el));
	std::string *ctx = d->ctx.back();

	if(ctx && *ctx == "item")
		for(int i=0;attr[i];i+=2)
			if(!stricmp(attr[i],"href"))
				d->manifest.push_back(new std::string(attr[i+1]));
}

void epub_rootfile_end(void *data, const char *el) {
   	epub_data_t *d = (epub_data_t*)data;
	d->ctx.pop_back();
}

void epub_rootfile_char(void *data, const XML_Char *txt, int len) {
   	epub_data_t *d = (epub_data_t*)data;
	std::string *ctx = d->ctx.back();

	if(ctx && *ctx == "dc:title") {
		XML_Char *buf = new XML_Char[len+1];
		strncpy(buf,txt,len);
		std::string s = buf;
		// eyeballed with the current browser font
		// to avoid overruning buttonwidth.
		// FIXME buttons should format their own 
		d->title = s.substr(0,26);
		if(s.length() > 26) d->title.append("...");
	  	delete buf;
	}
	else if(ctx && *ctx == "dc:creator") {
		XML_Char *buf = new XML_Char[len+1];
		strncpy(buf,txt,len);
		std::string s = buf;
		// eyeballed with the current browser font
		// to avoid overruning buttonwidth.
		// FIXME buttons should format their own 
		d->creator = s.substr(0,26);
		if(s.length() > 26) d->creator.append("...");
	  	delete buf;
	}
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
		XML_SetElementHandler(p, epub_rootfile_start, epub_rootfile_end);
		XML_SetCharacterDataHandler(p, epub_rootfile_char);
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
	} while (len);
	sprintf(msg,"info : read %d bytes.\n",len_total); Log(msg);
	XML_ParserFree(p);
	delete filebuf;
	return(rc);
}

//! Parse EPUB file. Set metadataonly to true if you
//! only want the title and author.
int epub(Book *book, std::string name, bool metadataonly)
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

	// Stop here if only metadata is required.
	if(metadataonly) {
	  if(parsedata.title.length())
	    book->SetTitle(parsedata.title.c_str());
	  unzClose(uf);
	  epub_data_delete(&parsedata);
	  return rc;
	}

	// Read all the XHTML listed, in sequence.
	parsedata.ctx.clear();
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

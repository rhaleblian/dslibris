/* 

dslibris - an ebook reader for the Nintendo DS.

 Copyright (C) 2007-2008 Ray Haleblian

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include "epub.h"
#include <nds.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include "main.h"
#include "parse.h"
#include "expat.h"
#include "zlib.h"
#include "unzip.h"
#include "log.h"

void epub_data_init(epub_data_t *d)
{
	d->type = PARSE_CONTAINER;
	d->ctx.clear();
	d->ctx.push_back(new std::string("TOP"));
	d->rootfile = "";
	d->title = "";
	d->creator = "";
	d->metadataonly = false;
	d->book = NULL;
}

void epub_data_delete(epub_data_t *d)
{
	std::vector<std::string*>::iterator it;
	d->manifest.clear();
	d->spine.clear();
	while(d->ctx.back()) d->ctx.pop_back();
}

void epub_container_start(void *data, const char *el, const char **attr)
{
	epub_data_t *d = (epub_data_t*)data;
	if(!strcmp(el,"rootfile"))
		for(int i=0;attr[i];i+=2)
			if(!strcmp(attr[i],"full-path"))
				d->rootfile = attr[i+1];
}

void epub_rootfile_start(void *data, const char *el, const char **attr) {
	epub_data_t *d = (epub_data_t*)data;
	std::string elem = el;
	std::string *ctx = d->ctx.back();
	if (!ctx) return;
	
	if(	(*ctx == "manifest" || *ctx == "opf:manifest")
	&& 	(elem == "item" || elem == "opf:item") ) {
		epub_item *item = new epub_item;
		d->manifest.push_back(item);
		for(int i=0;attr[i];i+=2) {
			if(!strcmp(attr[i],"id"))
				item->id = attr[i+1];
			if(!strcmp(attr[i],"href"))
				item->href = attr[i+1];
		}
	}

	else if (	(*ctx == "spine" || *ctx == "opf:spine")
	&& 	(elem == "itemref" || elem == "opf:itemref") )
	{
		epub_itemref *itemref = new epub_itemref;
		d->spine.push_back(itemref);
		for(int i=0;attr[i];i+=2) {
			if(!strcmp(attr[i],"idref"))
				itemref->idref = attr[i+1];
		}
	}

	else if ( elem == "dc:title" ) {
		d->title.clear();
	}
	
	else if ( elem == "dc:creator" ) {
		d->creator.clear();
	}
	
	d->ctx.push_back(new std::string(elem));
}

void epub_rootfile_end(void *data, const char *el) {
   	epub_data_t *d = (epub_data_t*)data;
	d->ctx.pop_back();
}

void epub_rootfile_char(void *data, const XML_Char *txt, int len) {
   	epub_data_t *d = (epub_data_t*)data;
	std::string *ctx = d->ctx.back();
	if (!ctx) return;
	
	if ( *ctx == "dc:title" ) {
		d->title.append((char*)txt,len);
	}
	else if ( *ctx == "dc:creator" ) {
		d->creator.append((char*)txt,len);
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
		sprintf(msg,"progr: %d byte chunk\n",len); Log(msg);
	} while (len);
	sprintf(msg,"info : read %d bytes total\n",len_total); Log(msg);
	XML_ParserFree(p);
	delete filebuf;
	return(rc);
}

int epub(Book *book, std::string name, bool metadataonly)
{
	//! Parse EPUB file.
	//! Set metadataonly to true if you only want the title and author.
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

	Log("progr: parsing rootfile\n");

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
		Log("progr: metadata only\n");
		if(parsedata.title.length()) {
			book->SetTitle(parsedata.title.c_str());
			if(parsedata.creator.length())
				book->SetAuthor(parsedata.creator);
		}
		unzClose(uf);
		epub_data_delete(&parsedata);
		return rc;
	}

	Log("progr: ordering sections\n");

	// Read the XHTML in the manifest, ordering by spine if needed.
	parsedata.ctx.clear();
	parsedata.book = book;
	parsedata.type = PARSE_CONTENT;
	vector<std::string*> href;
	if(parsedata.spine.size()) {
		// Use spine for reading order.
		Log("progr: ordering by spine\n");
		vector<epub_itemref*>::iterator itemref;
		for(itemref=parsedata.spine.begin();
			itemref!=parsedata.spine.end();
			itemref++) {
			vector<epub_item*>::iterator item;
			for(item=parsedata.manifest.begin();
				item!=parsedata.manifest.end();
				item++) {
				if((*item)->id == (*itemref)->idref) {
					std::string *h = new std::string((*item)->href);
					href.push_back(h);
				}
			}
		}
	}
	else {
		Log("progr: ordering by manifest\n");
		vector<epub_item*>::iterator item;
		for(item=parsedata.manifest.begin();
			item!=parsedata.manifest.end();
			item++)
			href.push_back(new std::string((*item)->href));
	}
	
	Log("progr: catenating sections\n");
	
	vector<std::string*>::iterator it;
	for(it=href.begin();it!=href.end();it++)
	{
		size_t pos = (*it)->find_last_of('.');
		if (pos < (*it)->length())
		{
			std::string ext = (*it)->substr(pos);
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
	unzClose(uf);
	epub_data_delete(&parsedata);
	return rc;
}

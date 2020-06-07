#ifndef _EPUB_H_
#define _EPUB_H_

#include "book.h"
#include <string>

typedef enum { PARSE_CONTAINER, PARSE_ROOTFILE, PARSE_CONTENT } epub_parse_t;

// <manifest> elements
typedef struct {
	std::string id;
	std::string href;
} epub_item;

// <spine> elements
typedef struct {
	std::string idref;
} epub_itemref;

typedef struct {
	epub_parse_t type;
	vector<std::string*> ctx;
	std::string rootfile;
	std::vector<epub_item*> manifest;
	std::vector<epub_itemref*> spine;
	Book *book;
	bool metadataonly;
	std::string title;
	std::string creator;
} epub_data_t;

int epub(Book *book, std::string filepath, bool metadataonly);

#endif

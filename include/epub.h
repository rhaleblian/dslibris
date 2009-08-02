#ifndef _EPUB_H_
#define _EPUB_H_

#include "Book.h"
#include <string>

typedef enum { PARSE_CONTAINER, PARSE_ROOTFILE, PARSE_CONTENT } epub_parse_t;
typedef struct {
	epub_parse_t type;
	vector<std::string*> ctx;
	std::string rootfile;
	std::vector<std::string*> manifest;
	Book *book;
  bool metadataonly;
  std::string title;
  std::string creator;
} epub_data_t;

int epub(Book *book, std::string filepath, bool metadataonly);

#endif

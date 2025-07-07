#ifndef BOOK_H
#define BOOK_H

#include "page.h"
#include <nds.h>
#include <string>
#include <list>
#include <vector>

typedef enum {FORMAT_UNDEF, FORMAT_XHTML, FORMAT_EPUB} format_t;

//! Encapsulates metadata and Page vector for a single book.

//! Bookmarks are in here too.
//! App maintains a vector of Book to represent the available library.

class Book {
	std::string filename;
	std::string foldername;
	std::string title;
	std::string author;
	int position;  //! as page index.
	std::list<uint16_t> bookmarks;  //! as page indices.
	std::vector<class Page*> pages;
public:
	Book();
	~Book();
	format_t format;
	inline std::string* GetAuthor() { return &author; }
	std::list<uint16_t>* GetBookmarks(void);
	int  GetNextBookmark(void);
	int  GetPreviousBookmark(void);
	int  GetNextBookmarkedPage(void);
	int  GetPreviousBookmarkedPage(void);
	const char* GetFileName(void);
	const char* GetFolderName(void);
	Page* GetPage();
	Page* GetPage(int i);
	uint16_t  GetPageCount();
	int  GetPosition(void);
	int  GetPosition(int offset);
	const char* GetTitle();
	void SetAuthor(std::string &s);
	void SetFileName(const char *filename);
	void SetFolderName(const char *foldername);	
	void SetFolderName(std::string &foldername);
	void SetPage(uint16_t index);
	void SetPosition(int pos);
	void SetTitle(const char *title);
	Page* AppendPage();
	Page* AdvancePage();
	Page* RetreatPage();
	void Cache();
	void Close();
	u8   Index();
	void IndexHTML();
	u8   Open();
	u8   Parse(bool fulltext);
	int  ParseHTML();
	void Restore();
};

#endif


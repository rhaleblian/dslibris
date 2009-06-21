#ifndef BOOK_H
#define BOOK_H

#include "Page.h"
#include <nds.h>
#include <string>
#include <list>
#include <vector>

//! Encapsulates metadata for a single book.
//! App maintains n of these to represent the available library.

class Book {
	std::string filename;
	std::string foldername;
	std::string title;
	std::string author;
	int position; //! Index of current page.
    std::list<u16> bookmarks;
	std::vector<class Page*> pages;
public:
	Book();
	~Book();
    std::list<u16>* GetBookmarks(void);
	const char* GetFileName(void);
	const char* GetFolderName(void);
	Page* GetPage();
	Page* GetPage(int i);
	u16  GetPageCount();
	int  GetPosition(void);
	const char* GetTitle();
	void SetFileName(const char *filename);
	void SetFolderName(const char *foldername);	
	void SetFolderName(std::string &foldername);
	void SetPage(u16 index);
	void SetPosition(int pos);
	void SetTitle(const char *title);
	Page* AppendPage();
	Page* AdvancePage();
	Page* RetreatPage();
	void Close();
	u8   Index();
	void IndexHTML();
	u8   Open();
	u8   Parse(bool fulltext);
	int  ParseHTML();
};

#endif


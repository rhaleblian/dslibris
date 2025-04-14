#pragma once

#include <string>
#include <list>
#include <nds.h>
#include <vector>
#include "parse.h"

class App;
class Page;

typedef enum {FORMAT_UNDEF, FORMAT_XHTML, FORMAT_EPUB} format_t;

//! Encapsulates metadata and Page vector for a single book.

//! Bookmarks are in here too.
//! App maintains a vector of Book to represent the available library.

class Book {
	public:
	Book(App *app);
	~Book();
	inline std::string* GetAuthor() { return &author; }
	std::list<u16>* GetBookmarks(void);
	int  GetNextBookmark(void);
	int  GetPreviousBookmark(void);
	int  GetNextBookmarkedPage(void);
	int  GetPreviousBookmarkedPage(void);
	const char* GetFileName(void);
	const char* GetFolderName(void);
	Page* GetPage();
	Page* GetPage(int i);
	u16  GetPageCount();
	int  GetPosition(void);
	int  GetPosition(int offset);
	const char* GetTitle();
	void HandleEvent();
	void SetAuthor(std::string &s);
	void SetFileName(const char *filename);
	void SetFolderName(const char *foldername);	
	void SetFolderName(std::string &foldername);
	void SetPage(u16 index);
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
	App *app;
	format_t format;

	private:
	std::string filename;
	std::string foldername;
	std::string title;
	std::string author;
	int position;  //! as page index.
	std::list<u16> bookmarks;  //! as page indices.
	std::vector<Page*> pages;
	parsedata_t parsedata; 	//! user data block passed to expat callbacks.
};

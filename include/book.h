#pragma once

#include <string>
#include <list>
#include <nds.h>
#include <vector>
#include "app.h"
#include "page.h"

typedef enum {FORMAT_UNDEF, FORMAT_XHTML, FORMAT_EPUB} format_t;

namespace xml::book {
	void start(void *data, const char *el, const char **attr);
	void chardata(void *data, const char *txt, int txtlen);
	void end(void *data, const char *el);
	void instruction(void *data, const char *target, const char *pidata);
	int  unknown(void *encodingHandlerData, const XML_Char *name, XML_Encoding *info);
	void fallback(void *data, const XML_Char *s, int len);
}

namespace xml::book::metadata {
	void start(void *userdata, const char *el, const char **attr);
	void chardata(void *userdata, const char *txt, int txtlen);
	void end(void *userdata, const char *el);
}

//! Encapsulates metadata and Page vector for a single book.

//! Bookmarks are in here too.
//! App maintains a vector of Book to represent the available library.

class Book {
	std::string filename;
	std::string foldername;
	std::string title;
	std::string author;
	int position;  //! as page index.
	std::list<u16> bookmarks;  //! as page indices.
	std::vector<class Page*> pages;
	App *app;  //! pointer to the App instance.
public:
	Book(App *app);
	~Book();
	format_t format;
	inline App* GetApp() { return app; }
	inline std::string GetAuthor() { return author; }
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
	u8   Parse(bool fulltext=true);
	int  ParseHTML();
	void Restore();
};

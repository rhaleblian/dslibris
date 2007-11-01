#ifndef _book_h
#define _book_h

#include <nds.h>
#include <string>

class Book {
	std::string filename;
	std::string title;
	std::string author;
	u16 position;

public:
	Book();
	const char* GetFilename();
	s16  GetPosition();
	const char *GetTitle();
	void Index(char *filebuf);	
	void IndexHTML(char *filebuf);	
	u8   Parse(char *filebuf);
	u8   ParseHTML(char *filebuf);
	void SetFilename(const char *filename);
	void SetPosition(s16 pos);
	void SetTitle(const char *title);
};

#endif

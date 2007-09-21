#ifndef _book_h
#define _book_h

#include <nds.h>
#include <string>

class Book {
	std::string filename;
	std::string title;
	std::string author;
	s16 position;

public:
	Book();
	const char* GetFilename();
	s16 GetPosition();
	const char *GetTitle();
	void SetFilename(const char *filename);
	void SetTitle(const char *title);
	void SetPosition(s16 pos);
	void Index();	
	u8 Parse(char *filebuf);
};

#endif


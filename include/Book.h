#ifndef BOOK_H
#define BOOK_H

#include <nds.h>
#include <string>

class Book {
	std::string filename;
	std::string foldername;
	std::string title;
	std::string author;
	u16 position;

public:
	Book();
	const char* GetFileName(void);
	const char* GetFolderName(void);
	s16  GetPosition(void);
	const char* GetTitle();
	u8   Index(char *filebuf);	
	void IndexHTML(char *filebuf);	
	u8   Parse(char *filebuf);
	u8   ParseHTML(char *filebuf);
	void SetFileName(const char *filename);
	void SetFolderName(const char *foldername);	
	void SetPosition(s16 pos);
	void SetTitle(const char *title);
};

#endif


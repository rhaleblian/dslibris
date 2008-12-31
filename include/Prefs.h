#ifndef _PREFS_H
#define _PREFS_H
#include "expat.h"

class Prefs
{
public:
	class App *app;
	Prefs();
	Prefs(class App *parent);
	~Prefs();
	int Read(XML_Parser p);
	int Read();
	int Write();
};
#endif

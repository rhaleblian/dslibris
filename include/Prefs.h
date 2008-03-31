#ifndef _PREFS_H
#define _PREFS_H
#include "expat.h"
#include "App.h"

class Prefs
{
public:
	App *app;
	Prefs(App *parent);
	~Prefs();
	bool Read(XML_Parser p);
	bool Write();
};
#endif

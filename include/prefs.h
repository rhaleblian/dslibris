#pragma once

#include "expat.h"
#include "app.h"

class Prefs
{
	public:
	Prefs(App *app);
	~Prefs();
	void Apply();
	int Read();
	int Write();
	long modtime;
	bool swapshoulder;

	private:
	App *app;
	void Init();
};

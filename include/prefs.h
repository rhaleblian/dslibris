#pragma once

class App;

class Prefs
{
	public:
	App *app;
	long modtime;
	Prefs();
	Prefs(App *parent);
	~Prefs();
	int Read();
	int Write();
	bool swapshoulder;

	private:
	void Init();
};

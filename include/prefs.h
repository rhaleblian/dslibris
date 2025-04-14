#ifndef _PREFS_H
#define _PREFS_H

class App;

class Prefs
{
	public:
	Prefs(class App *app);
	~Prefs();	
	void Init();
	int Read();
	int Write();
	long modtime;
	bool swapshoulder;

	private:
	App *app;
};

#endif // _PREFS_H

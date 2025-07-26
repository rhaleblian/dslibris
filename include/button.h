#ifndef _button_h
#define _button_h

#include "text.h"
#include <unistd.h>
#include <string>

enum {
	BUTTON_STYLE_SETTING,
	BUTTON_STYLE_BOOK
};

typedef struct {
	u16 x;
	u16 y;
} coord_t;

//! A very simple button with a text label.

class Button {

	private:

	coord_t origin;
	coord_t extent;
	bool draw_border;
	int style;
	std::string text;
	//! intended for the author name.
	std::string text2;
	Text *ts;

    public:

	Button();
	Button(Text *typesetter);
	inline int GetHeight() { return extent.y; };
	inline const char* GetLabel() { return text.c_str(); };
	void Init(Text *typesetter);
	void Label(const char *text);
	//! label on first line, used for title in the library screen.
	inline void SetLabel(std::string &s) { SetLabel1(s); };
	void SetLabel1(std::string s);
	//! label on second line, used for author in the library screen.
	void SetLabel2(std::string s);
	inline void SetStyle(int astyle) { style = astyle; };
	void Draw(u16 *fb, bool highlight);
	void Move(u16 x, u16 y);
	void Resize(u16 x, u16 y);
	bool EnclosesPoint(u16 x, u16 y);
};

#endif

#ifndef _button_h
#define _button_h

#include "text.h"
#include <unistd.h>
#include <string>

typedef struct {
	uint16_t x;
	uint16_t y;
} coord_t;

//! A very simple button with a text label.

class Button {
	coord_t origin;
	coord_t extent;
	std::string text;
	//! intended for the author name.
	std::string text2;
	Text *ts;

 public:
	Button();
	Button(Text *typesetter);
	inline const char* GetLabel() { return text.c_str(); };
	void Init(Text *typesetter);
	void Label(const char *text);
	void SetLabel(std::string &s);
	//! label on next line, used for author in the library screen.
	void SetLabel2(std::string s);
	void Draw(uint16_t *fb, bool highlight);
	void Move(uint16_t x, uint16_t y);
	void Resize(uint16_t x, uint16_t y);
	bool EnclosesPoint(uint16_t x, uint16_t y);
};

#endif

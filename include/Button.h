#ifndef _button_h
#define _button_h

#include "Text.h"
#include <unistd.h>

typedef struct {
	u16 x;
	u16 y;
} coord_t;

class Button {
	coord_t origin;
	coord_t extent;
	u8 text[MAXPATHLEN];
	Text *ts;

 public:
	Button();
	Button(Text *typesetter);
	void Init(Text *typesetter);
	void Label(const char *text);
	void Draw(u16 *fb, bool highlight);
	void Move(u16 x, u16 y);
	void Resize(u16 x, u16 y);
	bool EnclosesPoint(u16 x, u16 y);
};

#endif

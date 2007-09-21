#ifndef _button_h
#define _button_h

#include "Text.h"

typedef struct {
  u16 x;
  u16 y;
} coord_t;

class Button {
  coord_t origin;
  coord_t extent;
  u8 text[64];
  Text *ts;

 public:
  void Init(Text *typesetter);
  void Label(const char *text);
  void Draw(u16 *fb, bool highlight);
  void Move(u16 x, u16 y);
};

#endif

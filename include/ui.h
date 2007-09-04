#ifndef _UI_H_
#define _UI_H_

#include "text.h"

typedef struct {
  u16 x;
  u16 y;
} coord_t;

#ifdef _cplusplus
class Button {
  coord_t origin;
  coord_t extent;
  u8 text[64];
  Text *ts;

 public:
  void init(Text *typesetter);
  void label(char *text);
  void draw(u16 *fb, bool highlight);
  void move(u16 x, u16 y);
};
#else
typedef struct {
  coord_t origin;
  coord_t extent;
  u8 text[64];
} button_t;

  void button_init(button_t *b);
  void button_label(button_t *b, char *text);
  void button_draw(button_t *b, u16 *fb, bool highlight);
  void button_move(button_t *b, u16 x, u16 y);
#endif

#endif

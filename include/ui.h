#ifndef _ui_h
#define _ui_h

typedef struct {
  u16 x;
  u16 y;
} coord_t;

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

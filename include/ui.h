#ifndef _UI_H_
#define _UI_H_

typedef struct {
  u16 x;
  u16 y;
} coord_t;

typedef struct {
  coord_t origin;
  coord_t extent;
  u8 text[64];
} button_t;

void initbutton(button_t *b);
void labelbutton(button_t *b, char *text);
void drawbutton(button_t *b, u16 *fb, bool highlight);
void movebutton(button_t *b, u16 x, u16 y);

#endif

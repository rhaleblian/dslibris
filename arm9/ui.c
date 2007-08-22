#include <nds.h>
#include <stdio.h>
#include "ui.h"
#include "font.h"

void initbutton(button_t *b) {
  b->origin.x = 0;
  b->origin.y = 0;
  b->extent.x = 192;
  b->extent.y = 32;
  strcpy((char*)b->text, "");
}

void labelbutton(button_t *b, char *text) {
  strncpy((char*)b->text,(char*)text,63);
}

void movebutton(button_t *b, u16 x, u16 y) {
  b->origin.x = x;
  b->origin.y = y;
}

void drawbutton(button_t *b, u16 *fb, bool highlight) {
  u16 x; u16 y;
  coord_t ul, lr;
  u16 bordercolor = RGB15(28,28,28) | BIT(15);
  u16 locolor = RGB15(14,14,14) | BIT(15);
  ul.x = b->origin.x;
  ul.y = b->origin.y;
  lr.x = b->origin.x + b->extent.x;
  lr.y = b->origin.y + b->extent.y;

  for(x=ul.x;x<lr.x;x++) {
    fb[ul.y*SCREEN_WIDTH + x] = bordercolor;
    fb[lr.y*SCREEN_WIDTH + x] = bordercolor;
  }
  for(y=ul.y;y<lr.y;y++) {
    fb[y*SCREEN_WIDTH + ul.x] = bordercolor;
    fb[y*SCREEN_WIDTH + lr.x] = bordercolor;
  }

  if(highlight) {
    for(y=ul.y;y<ul.y+8;y++) {
      for(x=ul.x;x<ul.x+8;x++) {
	fb[y*SCREEN_WIDTH + x] = locolor;
      }
    }
  }

  tsGetPen(&x,&y);
  tsSetPen(ul.x+10, ul.y + tsGetHeight());
  tsString((char*)b->text);
  tsSetPen(x,y);
}

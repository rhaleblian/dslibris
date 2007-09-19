#include <nds.h>
#include <stdio.h>
#include "ui.h"
#include "font.h"

void button_init(button_t *b) {
  b->origin.x = 0;
  b->origin.y = 0;
  b->extent.x = 192;
  b->extent.y = 32;
  strcpy((char*)b->text, "");
}

void button_label(button_t *b, char *text) {
  strncpy((char*)b->text,(char*)text,63);
}

void button_move(button_t *b, u16 x, u16 y) {
  b->origin.x = x;
  b->origin.y = y;
}

void button_draw(button_t *b, u16 *fb, bool highlight) {
  u16 x; u16 y;
  coord_t ul, lr;
  ul.x = b->origin.x;
  ul.y = b->origin.y;
  lr.x = b->origin.x + b->extent.x;
  lr.y = b->origin.y + b->extent.y;

  if(highlight) {
    for(y=ul.y;y<lr.y;y++) {
      for(x=ul.x;x<lr.x;x++) {
	fb[y*SCREEN_WIDTH + x] = RGB15(31,31,31) | BIT(15);
      }
    }
  }
  
  u16 bordercolor = RGB15(15,15,15) | BIT(15);
  for(x=ul.x;x<lr.x;x++) {
    fb[ul.y*SCREEN_WIDTH + x] = bordercolor;
    fb[lr.y*SCREEN_WIDTH + x] = bordercolor;
  }
  for(y=ul.y;y<lr.y;y++) {
    fb[y*SCREEN_WIDTH + ul.x] = bordercolor;
    fb[y*SCREEN_WIDTH + lr.x] = bordercolor;
  }

  if(!highlight) tsSetInvert(true);
  tsGetPen(&x,&y);
  tsSetPen(ul.x+10, ul.y + tsGetHeight());
  tsString((char*)b->text);
  tsSetPen(x,y);
  if(!highlight) tsSetInvert(false);
}

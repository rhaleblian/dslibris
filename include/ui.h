#ifndef _UI_H_
#define _UI_H_
typedef struct {
	int x;
	int y;
} coord_t;

typedef struct {
	coord_t origin;
	coord_t extent;
	char text[64];
} button_t;
void initbutton(button_t *b);
void labelbutton(button_t *b, char *text);
void drawbutton(button_t *b, u16 *fb, int hightlight);
void movebutton(button_t *b, int x, int y);
#endif

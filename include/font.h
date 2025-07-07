#ifndef _FONT_H
#define _FONT_H

#include <ft2build.h>	/* freetype2 - text rendering */
#include FT_FREETYPE_H

void tsSetPixelSize(int size);
u8 tsAdvance(uint16_t code);
void tsChar(uint16_t code);
void tsDump(void);
void tsGetPen(uint16_t *x, uint16_t *y);
u8 tsGetPenX(void);
u8 tsGetPenY(void);
int tsInitDefault(void);
void tsInitPen(void);
u8 tsGetHeight(void);
int tsNewLine(void);
void tsSetPen(uint16_t x, uint16_t y);
void tsString(const char *string);
u8 ucs(const char *txt, uint16_t *code);
void tsSetInvert(bool state);
bool tsGetInvert(void);

#endif

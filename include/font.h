#pragma once

#include <ft2build.h>	/* freetype2 - text rendering */
#include FT_FREETYPE_H

void tsSetPixelSize(int size);
uint8_t tsAdvance(uint16_t code);
void tsChar(uint16_t code);
void tsDump(void);
void tsGetPen(uint16_t *x, uint16_t *y);
uint8_t tsGetPenX(void);
uint8_t tsGetPenY(void);
int tsInitDefault(void);
void tsInitPen(void);
uint8_t tsGetHeight(void);
int tsNewLine(void);
void tsSetPen(uint16_t x, uint16_t y);
void tsString(const char *string);
uint8_t ucs(const char *txt, uint16_t *code);
void tsSetInvert(bool state);
bool tsGetInvert(void);

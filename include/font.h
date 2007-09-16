#ifndef _FONT_H
#define _FONT_H

#include <ft2build.h>	/* freetype2 - text rendering */
#include FT_FREETYPE_H

void tsSetPixelSize(int size);
u8 tsAdvance(u16 code);
void tsChar(u16 code);
void tsDump(void);
inline void tsGetPen(u16 *x, u16 *y);
u8 tsGetPenX(void);
u8 tsGetPenY(void);
int tsInitDefault(void);
void tsInitPen(void);
u8 tsGetHeight(void);
int tsNewLine(void);
inline void tsSetPen(u16 x, u16 y);
void tsString(const char *string);
u8 ucs(const char *txt, u16 *code);
inline void tsSetInvert(bool state);
inline bool tsGetInvert(void);

#endif

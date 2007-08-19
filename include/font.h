#ifndef _FONT_H
#define _FONT_H

#include <ft2build.h>	/* freetype2 - text rendering */
#include FT_FREETYPE_H

u8 tsAdvance(u16 code);
int tsInitDefault(void);
void tsInitPen(void);
u8 tsGetHeight(void);
inline void tsGetPen(u16 *x, u16 *y);
inline void tsSetPen(u16 x, u16 y);
void tsChar(u16 code);
void tsString(char *string);
int tsStartNewLine(void);

#endif

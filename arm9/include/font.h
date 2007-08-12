#ifndef _FONT_H
#define _FONT_H

#include <ft2build.h>	/* freetype2 - text rendering */
#include FT_FREETYPE_H

u8 tsAdvance(u16 code);
int	tsInitDefault(void);
void	tsInitPen(void);
int  	tsGetHeight(void);
void	tsGetPen(int *x, int *y);
void	tsSetPen(int x, int y);
void	tsChar(int code);
void	tsString(u8 *string);
int	tsStartNewLine(void);

#endif

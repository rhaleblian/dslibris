#ifndef _font_h_
#define _font_h_

#include <ft2build.h>	/* freetype2 - text rendering */
#include FT_FREETYPE_H

void drawchar(int code, FT_Vector *pen);
void drawstring(char *string, FT_Vector *pen);

#endif // _font_h_

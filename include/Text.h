#ifndef _text_h
#define _text_h

#include <ft2build.h>
#include FT_FREETYPE_H

#define MAXGLYPHS 128
#define FONTFILENAME "dslibris.ttf"

#ifdef _cplusplus
class Text {
  FT_Library library;
  FT_Face face;
  FT_GlyphSlotRec glyphs[MAXGLYPHS];
  FT_Vector pen;
  FT_Error error;

 public:
  int InitDefault(void);
  u8 ucs(unsigned char *txt, u16 *code);
  u8 GetHeight(void);
  u8 Advance(u16 code);
  void Dump(void);

  void InitPen(void);
  void GetPen(u16 *x, u16 *y);
  void SetPen(u16 x, u16 y);

  void PrintChar(u16 code);
  void PrintString(char *string);
  int StartNewLine(void);
};
#endif

#endif


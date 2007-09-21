#ifndef _text_h
#define _text_h

#include <ft2build.h>
#include FT_FREETYPE_H

#define MAXGLYPHS 128
#define FONTFILENAME "dslibris.ttf"

class Text {
  FT_Library library;
  FT_Face face;
  FT_GlyphSlotRec glyphs[MAXGLYPHS];
  FT_Vector pen;
  FT_Error error;

 public:
  int  InitDefault(void);
  u8   GetUCS(unsigned char *txt, u16 *code);
  u8   GetHeight(void);
  bool GetInvert();
  void SetInvert(bool);

  void InitPen(void);
  void GetPen(u16 *x, u16 *y);
  void SetPen(u16 x, u16 y);
  u16 GetPenX();
  u16 GetPenY();

  void PrintChar(u16 code);
  void PrintString(char *string);
  int  PrintNewLine(void);

  u8   Advance(u16 code);
  void Dump(void);
};

#endif


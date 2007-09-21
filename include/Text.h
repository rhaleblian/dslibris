#ifndef _text_h
#define _text_h

#include <ft2build.h>
#include FT_FREETYPE_H

#define MAXGLYPHS 128
#define FONTFILENAME "dslibris.ttf"

#define PIXELSIZE 10
#define DPI 72	/** probably not true for a DS - measure it **/
#define EMTOPIXEL (float)(POINTSIZE * DPI/72.0)

class Text {
  FT_Library library;
  FT_Face face;
  FT_GlyphSlotRec glyphs[MAXGLYPHS];
  FT_Vector pen;
  FT_Error error;
	bool usecache;
	bool invert;

 public:
	int  InitDefault(void);
  u8   GetUCS(const char *txt, u16 *code);
  u8   GetHeight(void);
  bool GetInvert();
  void SetInvert(bool);
	void SetPixelSize(u8);
	u8 GetPixelSize();

  void InitPen(void);
  void GetPen(u16 *x, u16 *y);
  void SetPen(u16 x, u16 y);
  u8 GetPenX();
  u8 GetPenY();

  void PrintChar(u16 code);
  void PrintString(const char *string);
  int  PrintNewLine(void);

  u8   Advance(u16 code);
  void Dump(void);
};

#endif


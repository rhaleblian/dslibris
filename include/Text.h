#ifndef _text_h
#define _text_h

#include <string>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H

using namespace std;

#define EMTOPIXEL (float)(POINTSIZE * DPI/72.0)
#define FONTFILENAME "dslibris.ttf"
#define CACHESIZE 512
#define PIXELSIZE 10
#define DPI 72	/** probably not true for a DS - measure it **/

class Text {
	FT_Library library;
	FT_Face face;
	FT_GlyphSlotRec glyphs[CACHESIZE];
	FT_Vector pen;
	FT_Error error;

	// associates each glyph cache index (value)
	// with it's Unicode code point (key).
	map<u16,u16> cachemap;
	u16 cachenext;

	string fontfilename;
	u16 *screen, *screenleft, *screenright;
	u8 pixelsize;
	bool invert;
	bool justify;
	
public:
	Text();
	int  InitDefault(void);
	void InitPen(void);

	u8   GetAdvance(u16 code);
	u8   GetCharCode(const char *txt, u16 *code);
	u8   GetHeight(void);
	bool GetInvert();
	void GetPen(u16 *x, u16 *y);
	u8   GetPenX();
	u8   GetPenY();
	u8   GetPixelSize();
	u16* GetScreen();
	u8   GetStringWidth(const char *txt);

	void SetInvert(bool);
	void SetPen(u16 x, u16 y);
	void SetPixelSize(u8);
	void SetScreen(u16 *s);

	FT_GlyphSlot CacheGlyph(u16 codepoint);

	void ClearRect(u16 xl, u16 yl, u16 xh, u16 yh);
	void ClearScreen();

	void PrintChar(u16 code);
	bool PrintNewLine(void);
	void PrintStatusMessage(const char *msg);
	void PrintString(const char *string);
};

#endif


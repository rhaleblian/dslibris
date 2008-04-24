#ifndef _text_h
#define _text_h

#include <string>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

using namespace std;

#define EMTOPIXEL (float)(POINTSIZE * DPI/72.0)
#define CACHESIZE 256
#define PIXELSIZE 12
#define DPI 72	/** probably not true for a DS - measure it **/

class App;

typedef struct TextFaceRec_ {
	const char* file_path;
	int face_index;
} TextFaceRec, *TextFace;

typedef struct TextCache_ {
	FTC_Manager manager;
	FTC_CMapCache cmap;
	FTC_ImageCache image;
	FTC_SBitCache sbit;
} TextCache;

class Text {
	FT_Library library;
	FT_Face face;
	FT_Error error;

	bool ftc;
	TextCache cache;
	TextFaceRec face_id;
	FTC_SBit sbit;
	FTC_ImageTypeRec imagetype;
	FT_Int charmap_index;

	FT_GlyphSlotRec glyphs[CACHESIZE];	
	u16 cache_ucs[CACHESIZE];
	// associates each glyph cache index (value)
	// with it's Unicode code point (key).
	map<u16,u16> cachemap;
	u16 cachenext;

	string fontfilename;
	u16 *screen, *screenleft, *screenright;
	u8 pixelsize;
	FT_Vector pen;
	bool invert;
	bool justify;
	u32 codeprev; // last printed char code

	int CacheGlyph(u32 ucs);
	int InitDefault();
	int InitWithCacheManager();
	FT_GlyphSlot GetGlyph(u32 ucs, int flags);
	FT_Error GetGlyphBitmap(u32 ucs, FTC_SBit *asbit);
	FT_UInt GetGlyphIndex(u32 ucs);

public:
	App *app;

	Text();
	int  Init();
	void InitPen(void);

	u8   GetAdvance(u32 ucs);
	u8   GetCharCode(const char* txt, u32* code);
	u8   GetHeight(void);
	bool GetInvert();
	void GetPen(u16 *x, u16 *y);
	void GetPen(u16 &x, u16 &y);
	u8   GetPenX();
	u8   GetPenY();
	u8   GetPixelSize();
	u16* GetScreen();
	u8   GetStringWidth(const char *txt);

	void SetInvert(bool invert);
	void SetPen(u16 x, u16 y);
	void SetPixelSize(u8 size);
	void SetScreen(u16 *s);

	void ClearCache();
	void ClearRect(u16 xl, u16 yl, u16 xh, u16 yh);
	void ClearScreen();
	void ClearScreen(u16*, u8, u8, u8);

	void PrintChar(u32 ucs);
	bool PrintNewLine(void);
	void PrintStatusMessage(const char *msg);
	void PrintString(const char *string);
	void PrintSplash(u16 *screen);
};

#endif


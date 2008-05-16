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
//#define DPI 72	/** probably not true for a DS - measure it **/
#define DPI 110 /** Reference: http://www.displaymate.com/psp_ds_shootout.htm **/
#define TEXT_BOLD 2
#define TEXT_ITALIC 3

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

class Cache {
public:
	// associates each glyph cache index (value)
	// with it's Unicode code point (key).
	map<u16, FT_GlyphSlot> cacheMap;
	u16 cachenext;
	
	Cache() {
		cachenext = 0;
	}
};

class Text {
	FT_Library library;
	FT_Error error;

	bool ftc;
	TextCache cache;
	TextFaceRec face_id;
	FTC_SBit sbit;
	FTC_ImageTypeRec imagetype;
	FT_Int charmap_index;

	/*
	FT_GlyphSlotRec glyphs[CACHESIZE];	
	u16 cache_ucs[CACHESIZE];
	// associates each glyph cache index (value)
	// with it's Unicode code point (key).
	map<u16,u16> cachemap;
	u16 cachenext;
	*/
	
	map<FT_Face, Cache*> textCache;
	
	string fontfilename;
	string fontBoldFilename;
	string fontItalicFilename;
	u16 *screen, *screenleft, *screenright;
	FT_Vector pen;
	bool invert;
	bool justify;
	u32 codeprev; // last printed char code

	int CacheGlyph(u32 ucs);
	int CacheGlyph(u32 ucs, FT_Face face);
	int InitDefault();
	int InitWithCacheManager();
	FT_GlyphSlot GetGlyph(u32 ucs, int flags);
	FT_GlyphSlot GetGlyph(u32 ucs, int flags, FT_Face face);
	FT_Error GetGlyphBitmap(u32 ucs, FTC_SBit *asbit);
	FT_UInt GetGlyphIndex(u32 ucs);

public:
	App *app;
	u8 pixelsize;
	FT_Face face;
	FT_Face boldFace;
	FT_Face italicFace;

	Text();
	~Text();
	int  Init();
	void InitPen(void);

	u8   GetAdvance(u32 ucs);
	u8   GetAdvance(u32 ucs, FT_Face face);
	u8   GetCharCode(const char* txt, u32* code);
	string GetFontFile();
	string GetFontBoldFile();
	string GetFontItalicFile();
	u8   GetHeight(void);
	bool GetInvert();
	void GetPen(u16 *x, u16 *y);
	void GetPen(u16 &x, u16 &y);
	u8   GetPenX();
	u8   GetPenY();
	u8   GetPixelSize();
	u16* GetScreen();
	u8   GetStringWidth(const char *txt);
	u8   GetStringWidth(const char *txt, FT_Face face);

	void SetInvert(bool invert);
	void SetPen(u16 x, u16 y);
	void SetPixelSize(u8 size);
	void SetFontFile(const char *filename, u8 style);
	void SetFontBoldFile(const char *filename, u8 style);
	void SetFontItalicFile(const char *filename, u8 style);
	void SetScreen(u16 *s);

	void ClearCache();
	void ClearCache(FT_Face face);
	void ClearRect(u16 xl, u16 yl, u16 xh, u16 yh);
	void ClearScreen();
	void ClearScreen(u16*, u8, u8, u8);

	void PrintChar(u32 ucs);
	void PrintChar(u32 ucs, FT_Face face);
	bool PrintNewLine(void);
	void PrintStatusMessage(const char *msg);
	void PrintString(const char *string);
	void PrintString(const char *string, FT_Face face);
	void PrintSplash(u16 *screen);
};

#endif


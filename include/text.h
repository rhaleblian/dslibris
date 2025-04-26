#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include <map>
#include <string>
#include <vector>
#include "button.h"
#include "define.h"

class App;
class Button;

typedef struct TextFaceRec_ {
	char file_path[128];
	int face_index;
} TextFaceRec, *TextFace;

typedef struct TextCache_ {
	FTC_Manager manager;
	FTC_CMapCache cmap;
	FTC_ImageCache image;
	FTC_SBitCache sbit;
} TextCache;

//! Homemade glyph cache.

//! Fetches from FreeType can be expensive,
//! so keep advance and gylphs handy,
//! keyed by UCS codepoint.

class Cache {
public:
	//! Associates each glyph cache index (value)
	//! with its Unicode code point (key).
	std::map<u16, FT_GlyphSlot> cacheMap;
	//! ??
	u16 cachenext;
	
	Cache() {
		cachenext = 0;
	}
};

//! Typesetter singleton that provides all text rendering services.

//! Implemented atop FreeType 2.
//! Attempts to cache for draw performance using a homemade cache.
//! The code using FreeType's cache is inoperative.

class Text {
	public:
	Text(App *app);
	~Text();

	int pixelsize;
	//! Not used ... really.
	struct { u8 r; u8 g; u8 b; } bgcolor;
	bool usebgcolor;
	//! Pointers to screens and which one is current.
	u16 *screen, *screenleft, *screenright;
	//! Offscreen buffer. Only used when OFFSCREEN defined.
	u16 *offscreen;
	struct {
		int left, right, top, bottom;
	} margin;
	struct {
		int width, height;
	} display;
	int linespacing;
	u8 orientation;
	u8 paraspacing, paraindent;
	bool linebegan, bold, italic;

	int  Init();
	void InitPen(void);
	void Begin();
	void End();
	
	u8   GetAdvance(u32 ucs);
	u8   GetAdvance(u32 ucs, u8 style);
	u8   GetCharCode(const char* txt, u32* code);
	u8   GetCharCountInsideWidth(const char *txt, u8 style, u8 pixels);
	FT_Face GetFace() { return face; }
	FT_Face GetFace(u8 style) { return faces[style]; }
	std::string GetFontFile(u8 style);
	bool GetFontName(std::string &s);
	u8   GetHeight(void);
	bool GetInvert();
	void GetPen(u16 *x, u16 *y);
	void GetPen(u16 &x, u16 &y);
	u8   GetPenX();
	u8   GetPenY();
	u8   GetPixelSize();
	u16* GetScreen();
	int  GetStringAdvance(const char *txt);
	u8   GetStringWidth(const char *txt, u8 style);
	inline int GetStyle() { return style; }

	void SetInvert(bool invert);
	void SetPen(u16 x, u16 y);
	void SetPixelSize(u8 size);
	bool SetFace(u8 style);
	void SetFontFile(const char *filename, u8 style);
	void SetScreen(u16 *s);
	inline void SetStyle(int astyle) { style = astyle; face = faces[style]; }
	
	void ClearCache();
	void ClearCache(u8 style);
	void ClearRect(u16 xl, u16 yl, u16 xh, u16 yh);
	void ClearScreen();
	void ClearScreen(u16*, u8, u8, u8);
	void CopyScreen(u16 *src, u16 *dst);
	void SwapScreens();

	void PrintChar(u32 ucs);
	void PrintChar(u32 ucs, u8 style);
	bool PrintNewLine(void);
	void PrintStats();
	void PrintStatusMessage(const char *msg);
	void PrintString(const char *string);
	void PrintString(const char *string, u8 style);
	void PrintSplash(u16 *screen);
	inline void ReportFace(int style) { ReportFace(faces[style]); };
	void ReportFaces();

	App *app;
	
	FT_Library library;
	FT_Error error;
	//! Use the FreeType cache?
	bool ftc;

	// A: Homemade cache
	TextCache cache;
	TextFaceRec face_id;
	std::map<FT_Face, Cache*> textCache;
	
	// B: FreeType cache subsystem
	FTC_SBitRec sbit;
	FTC_ImageTypeRec imagetype;
	FT_Int charmap_index;
	
	std::map<u8, FT_Face> faces;
	std::map<u8, std::string> filenames;

	//! Current style, as TEXT_FONT_STYLE.
	int style;
	//! Current face.
	FT_Face face;
	//! Current draw position.
	FT_Vector pen;
	//! Draw light text on dark?
	bool invert;
	//! Last printed char code.
	u32 codeprev;
	//! Was the last glyph lookup a cache hit?
	bool hit;
	//! Has Init() run?
	bool initialized;

	//! It would fully justify, if it worked.
	bool justify;

	uint8_t fontSelected;
	uint8_t fontPage;
	std::vector<Button*>fontButtons;

	private:
	int CacheGlyph(u32 ucs);
	int CacheGlyph(u32 ucs, u8 style);
	int CacheGlyph(u32 ucs, FT_Face face);
	void ClearCache(FT_Face face);
	FT_Error CreateFace(int style);
	FT_GlyphSlot GetGlyph(u32 ucs, int flags);
	FT_GlyphSlot GetGlyph(u32 ucs, int flags, u8 style);
	FT_GlyphSlot GetGlyph(u32 ucs, int flags, FT_Face face);
	FT_Error GetGlyphBitmap(u32 ucs, FTC_SBit *asbit, FTC_Node *anode=NULL);
	FT_UInt GetGlyphIndex(u32 ucs);
	u8 GetAdvance(u32 ucs, FT_Face face);
	u8 GetStringWidth(const char *txt, FT_Face face);
	FT_Error InitFreeTypeCache();
	int InitHomemadeCache();
	void PrintChar(u32 ucs, FT_Face face);
	void PrintString(const char *string, FT_Face face);
	void ReportFace(FT_Face face);

	// Keep stats to check efficiency of caching.	
	//! Total glyph cache hits.
	int stats_hits;
	//! Total glyph cache misses.
	int stats_misses;
};

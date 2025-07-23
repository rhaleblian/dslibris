#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include <map>
#include <nds.h>
#include <string>

//! Reference: FreeType2 online documentation
#define EMTOPIXEL (float)(POINTSIZE * DPI/72.0)

//! Reference: http://www.displaymate.com/psp_ds_shootout.htm
#define DPI 110

#define FONTBOLDFILE "b.ttf"
#define FONTBOLDITALICFILE "bi.ttf"
#define FONTBROWSERFILE "m.ttf"
#define FONTITALICFILE "i.ttf"
#define FONTREGULARFILE "r.ttf"
#define FONTSPLASHFILE "s.ttf"

#define TEXT_BOLD_ON 2
#define TEXT_BOLD_OFF 3
#define TEXT_ITALIC_ON 4
#define TEXT_ITALIC_OFF 5
#define TEXT_STYLE_REGULAR (u8)0
#define TEXT_STYLE_BOLD (u8)1
#define TEXT_STYLE_ITALIC (u8)2
#define TEXT_STYLE_BROWSER (u8)3
#define TEXT_STYLE_SPLASH (u8)4

#define CACHESIZE 512
#define PIXELSIZE 12

//! Identifier to use as a FT_FaceID.
typedef struct TextFaceRec_ {
	char file_path[128];
	int face_index;
} TextFaceRec, *TextFace;

//! FTC caches.
typedef struct TextCache_ {
	FTC_Manager manager;
	FTC_CMapCache cmap;
	FTC_ImageCache image;
	FTC_SBitCache sbit;
} TextCache;

//! Singleton that provides all text rendering services,
//! implemented atop FreeType 2.
class Text {
	FT_Library library;
	FT_Error error;
	TextCache cache;
	FTC_SBitRec sbit;
	FTC_ImageTypeRec imagetype;
	FT_Int charmap_index;
	TextFaceRec styles[5];
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
	struct { u8 r; u8 g; u8 b; } bgcolor;
	void CopyBitmap(FT_BitmapGlyph glyph);
	FT_Face GetFace();
	FT_Face GetFace(u8 style);
	FT_GlyphSlot GetGlyph(u32 ucs, int flags = FT_RENDER_MODE_NORMAL);
	FT_Error GetGlyphBitmap(u32 ucs, FTC_SBit *asbit, FTC_Node *anode=NULL);
	FT_UInt GetGlyphIndex(u32 ucs);
	u8   GetAdvance(u32 ucs, FT_Face face);
	u8   GetStringWidth(const char *txt, FT_Face face);
	FT_Error InitFreeTypeCache();
	void ReportFace(FT_Face face);

  	public:

	class App *app;
	int  pixelsize;
	//! Pointers to screens and which one is current.
	u16  *screen, *screenleft, *screenright;
	//! Offscreen buffer. Only used when OFFSCREEN defined.
	u16  *offscreen;
	struct {
		int left, right, top, bottom;
	} margin;
	struct {
		int width, height;
	} display;
	int  linespacing;
	bool linebegan, bold, italic;
	bool usebgcolor;
	Text();
	Text(class App *parent) { app = parent; }
	~Text();
	int  Init();
	void InitPen(void);
	void Begin();
	void End();
	u8   GetAdvance(u32 charcode);
	u8   GetAdvance(u32 charcode, u8 style);
	u8   GetCharCode(const char* txt, u32* charcode);
	u8   GetCharCountInsideWidth(const char *txt, u8 style, u8 pixels);
	bool GetFontName(std::string &s);
	inline std::string GetFontFile(u8 astyle) { return styles[astyle].file_path; }
	u8   GetHeight();
	u8   GetPixelSize();
	inline int GetStyle() { return style; }
	void SetFont(u8 style, const char *filename);
	void SetPixelSize(u8 size);
	inline void SetStyle(u8 astyle) { style = astyle; }
	void ClearRect(u16 xl, u16 yl, u16 xh, u16 yh);
	void ClearScreen();
	void ClearScreen(u16*, u8, u8, u8);
	void CopyScreen(u16 *src, u16 *dst);
	u16* GetScreen();
	void SetScreen(u16 *s);
	void SwapScreens();
	bool GetInvert();
	void GetPen(u16 *x, u16 *y);
	void GetPen(u16 &x, u16 &y);
	u8   GetPenX();
	u8   GetPenY();
	int  GetStringAdvance(const char *txt);
	u8   GetStringWidth(const char *txt, u8 style);
	void PrintChar(u32 charcode);
	bool PrintNewLine();
	void PrintStats();
	void PrintStatusMessage(const char *msg);
	void PrintString(const char *string);
	void PrintSplash(u16 *screen);
	void SetInvert(bool invert);
	void SetPen(u16 x, u16 y);
};

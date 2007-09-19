#include <nds.h>
#include <fat.h>
#include "Text.h"
#include "main.h"

extern u16 *fb,*screen0,*screen1;

int Text::InitDefault(void) {
  if(FT_Init_FreeType(&library)) return 1;
  if(FT_New_Face(library, FONTFILENAME, 0, &face)) return 2;
  FT_Select_Charmap(face, FT_ENCODING_UNICODE);
  FT_Set_Pixel_Sizes(face, 0, PIXELSIZE);

  /** cache glyphs. glyphs[] will contain all the bitmaps.
      using FirstChar() and NextChar() would be more robust.
      TODO also cache kerning and transformations. **/

  FT_ULong  charcode;                                              
  FT_UInt   gindex;                                                
  charcode = FT_Get_First_Char( face, &gindex );                   
  while ( gindex != 0 )                                            
  {                                                                
    /** cache ASCII glyphs. **/
    if(charcode < MAXGLYPHS) {
      FT_Load_Char(face, charcode, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL);
      FT_GlyphSlot src = face->glyph;
      FT_GlyphSlot dst = &glyphs[charcode];
      int x = src->bitmap.rows;
      int y = src->bitmap.width;
      //dst->bitmap.buffer = (unsigned char*)malloc(x*y);
      dst->bitmap.buffer = new unsigned char[x*y];
      memcpy(dst->bitmap.buffer, src->bitmap.buffer, x*y);
      dst->bitmap.rows = src->bitmap.rows;
      dst->bitmap.width = src->bitmap.width;
      dst->bitmap_top = src->bitmap_top;
      dst->bitmap_left = src->bitmap_left;
      dst->advance = src->advance;
    }
    charcode = FT_Get_Next_Char( face, charcode, &gindex );        
  }                     

  usecache = true;
  invert = false;
  InitPen();
  return(0);
}

u8 Text::GetUCS(unsigned char *txt, u16 *code) {
  if(txt[0] > 0xc2 && txt[0] < 0xe0) {
    *code = ((txt[0]-192)*64) + (txt[1]-128);
    return 2;
    
  } else if(txt[0] > 0xdf && txt[0] < 0xf0) {
    *code = (txt[0]-224)*4096 + (txt[1]-128)*64 + (txt[2]-128);
    return 3;

  } else if(txt[0] > 0xef) {
    return 4;

  }
  *code = txt[0];
  return 1;
}

// accessors

u8 Text::GetHeight(void) { return (face->size->metrics.height >> 6); }

void Text::GetPen(u16 *x, u16 *y) { *x = pen.x; *y = pen.y; }

void Text::SetPen(u16 x, u16 y) { pen.x = x; pen.y = y; }

void Text::SetInvert(bool state) { invert = state; }
bool Text::GetInvert(void) { return invert; }

u8 Text::GetPenX(void) { return pen.x; }
u8 Text::GetPenY(void) { return pen.y; }

void Text::SetPixelSize(int size)
{
  if(!size) {
    FT_Set_Pixel_Sizes(face, 0, PIXELSIZE);
    usecache = true;
  } else {
    FT_Set_Pixel_Sizes(face, 0, size);
    usecache = false;
  }
}

// initialization

u8 Text::Advance(u16 code) {
  return glyphs[code].advance.x >> 6;
}

void Text::InitPen(void) {
  pen.x = MARGINLEFT;
  pen.y = MARGINTOP + (face->size->metrics.height >> 6);
}

/** drawing **/

void Text::PrintChar(u16 code) {
  /** draw a character with the current glyph
      into the current buffer at the current pen position. **/
  
  /** ASCII glyphs are cached; otherwise load. **/
  FT_GlyphSlot glyph;
  if(usecache && code < 128) glyph = &glyphs[code];
  else {
    FT_Load_Char(face, code, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL);
    glyph = face->glyph;
  }

  /** direct draw into framebuffer.**/
  FT_Bitmap bitmap = glyph->bitmap;
  u16 bx = glyph->bitmap_left;
  u16 by = glyph->bitmap_top;
  u16 gx, gy;
  for(gy=0; gy<bitmap.rows; gy++) {
    for(gx=0; gx<bitmap.width; gx++) {
      /* get antialiased value */
      u16 a = bitmap.buffer[gy*bitmap.width+gx];
      if(a) {
	u16 sx = (pen.x+gx+bx);
	u16 sy = (pen.y+gy-by);
	int l;
	if(invert) l = a >> 3;
	else l = (255-a) >> 3;
	fb[sy*SCREEN_WIDTH+sx] = RGB15(l,l,l) | BIT(15);
      }
    }
  }
  pen.x += glyph->advance.x >> 6;
}

int Text::PrintNewLine(void) {
  int height = face->size->metrics.height >> 6;
  pen.x = MARGINLEFT;
  pen.y += height + LINESPACING;
  if(pen.y > (PAGE_HEIGHT - MARGINBOTTOM)) {
    if(fb == screen0) {
      fb = screen1;
      pen.y = MARGINTOP + height;
    } else return(1);
  }
  return(0);
}

void Text::PrintString(const char *string) {
  // draw an ASCII string starting at the pen position.
  u8 i;
  for(i=0;i<strlen((char *)string);i++) {
    u16 c = string[i];
    if(c == '\n') PrintNewLine();
    else {
      if(c > 127) { i+=GetUCS((unsigned char*)&(string[i]),&c); i--; }
      PrintChar(c);
    }
  }
}

void Text::Dump(void) {
  int code;
  for(code=0;code<MAXGLYPHS;code++) {
    PrintChar(code);
    if(pen.x > 170) {
      pen.x = MARGINLEFT;
      pen.y += face->size->metrics.height >> 6;
    }
  }
}

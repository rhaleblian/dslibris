
// test modern versions of freetype2 against dslibris client code.
// portions from
// https://www.willusher.io/sdl2%20tutorials/2013/08/17/lesson-1-hello-world

#define ASCIIART // We are building under ARM without SDL

#ifndef ASCIIART
#include "SDL2/SDL.h"
#include "SDL2/SDL_render.h"
#endif
#include "ft2build.h"
#include <iostream>
#include FT_FREETYPE_H
#include FT_CACHE_H

#include "ft.h"

#define D 512

typedef struct {
  char *path;
  uint faceindex;
} Designator;

FT_Error requester(FTC_FaceID face_id, FT_Library library, FT_Pointer req_data,
                   FT_Face *aface) {
  auto designator = (Designator *)req_data;
  return FT_New_Face(library, designator->path, 0, aface);
}

FT_Freeables typesetter() {
  // char filepath[128] =
  //     "/home/ray/GitHub/dslibris/cflash/font/LiberationSerif-Regular.ttf";
  char filepath[128] = "/font/LiberationSerif-Regular.ttf";
  const uint char_code = 38; // ASCII &
  Designator designator;
  designator.path = filepath;
  designator.faceindex = 0;

  FTC_FaceID face_id = &designator;
  FT_Library library;
  FT_Face face;
  FTC_Manager manager;
  // FTC_ScalerRec scaler;
  FTC_ImageCache cache;
  FTC_ImageTypeRec imagetyperec;
  FTC_ImageType imagetype = &imagetyperec;
  FT_UInt glyph_index = 0;
  FT_Glyph glyph;
  FTC_Node node;
  FTC_CMapCache cmcache;

  FT_Error error = FT_Init_FreeType(&library);
  if (error)
    ; // shush g++.

  error =
      FTC_Manager_New(library, 1, 1, 1000000, requester, &designator, &manager);
  error = FTC_ImageCache_New(manager, &cache);
  error = FTC_CMapCache_New(manager, &cmcache);

  // Get the face.

  //  error = FT_New_Face(library, designator.path, 0, &face);
  error = FTC_Manager_LookupFace(manager, face_id, &face);

  // Set a size.

#if 0
  FT_Size_RequestRec rec;
  FT_Request_Size(face, &rec);
  error = FT_Set_Char_Size(face,    /* handle to face object           */
                           0,       /* char_width in 1/64th of points  */
                           16 * 64, /* char_height in 1/64th of points */
                           300,     /* horizontal device resolution    */
                           300);
  or
  error = FT_Set_Pixel_Sizes(
        face,   /* handle to face object */
        0,      /* pixel_width           */
        16 );   /* pixel_height          */

  Get the glyph index.

  char_code = FT_Get_First_Char(face, &glyph_index);
  glyph_index = FT_Get_Char_Index(face, char_code);
#endif
  glyph_index = FTC_CMapCache_Lookup(cmcache, face_id, 0, char_code);

  // Get the glyph fron the glyph index.

  // error = FT_Load_Glyph(face,                     /* handle to face object */
  //                       glyph_index,              /* glyph index           */
  //                       FT_LOAD_DEFAULT);         /* load flags, see below */
  imagetype->face_id = face_id;
  imagetype->flags = FT_LOAD_DEFAULT;
  imagetype->height = 16;
  imagetype->width = 0;
  error = FTC_ImageCache_Lookup(cache, imagetype, glyph_index, &glyph, &node);

  error = FT_Render_Glyph(face->glyph,            /* glyph slot  */
                          FT_RENDER_MODE_NORMAL); /* render mode */

  FT_Freeables f;
  f.face = face;
  f.library = library;
  f.manager = manager;
  return f;
}

#ifdef ASCIIART
int renderer(FT_Face face) {
  std::string s;
  auto bitmap = face->glyph->bitmap;
  for (uint y = 0; y < bitmap.rows; y++) {
    s.clear();
    for (uint x = 0; x < bitmap.width; x++) {
      uint v = bitmap.buffer[y * bitmap.width + x];
      if (v)
        s.append("&");
      else
        s.append(" ");
    }
    std::cerr << s << std::endl;
    iprintf(s.c_str());
  }
  return 0;
}
#else
int renderer(FT_Face face) {
  // Make something to draw things with.

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
    return 1;
  }

  SDL_Window *win =
      SDL_CreateWindow("FreeType Rendering", 100, 100, D, D, SDL_WINDOW_SHOWN);
  if (win == nullptr) {
    std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *ren = SDL_CreateRenderer(
      win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (ren == nullptr) {
    SDL_DestroyWindow(win);
    std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  SDL_Surface *surf = SDL_CreateRGBSurface(
      0, face->glyph->bitmap.width, face->glyph->bitmap.rows, 32, 0, 0, 0, 0);
  SDL_memcpy(surf->pixels, face->glyph->bitmap.buffer,
             face->glyph->bitmap.width * face->glyph->bitmap.rows);
  auto tex = SDL_CreateTextureFromSurface(ren, surf);

  // A sleepy rendering loop, wait for 3 seconds and render and present the
  // screen each time
  for (int i = 0; i < 3; ++i) {
    // First clear the renderer
    SDL_SetRenderDrawColor(ren, 127, 127, 127, 255);
    SDL_RenderClear(ren);
    // Draw the texture
    SDL_RenderCopy(ren, tex, NULL, NULL);
    // Update the screen
    SDL_RenderPresent(ren);
    // Take a quick break after all that hard work
    SDL_Delay(1000);
  }

  SDL_FreeSurface(surf);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}
#endif

void free_ft(FT_Freeables f) {
  FTC_Manager_Reset(f.manager);
  FTC_Manager_Done(f.manager);
  // FT_Done_Face(f.face);
  FT_Done_FreeType(f.library);
}

int ft_main(int argc, char **argv) {
  auto ft = typesetter();
  auto error = renderer(ft.face);
  free_ft(ft);
  return error;
}

// int SDL_BlitSurface(SDL_Surface*    src,
//                     const SDL_Rect* srcrect,
//                     SDL_Surface*    dst,
//                     SDL_Rect*       dstrect)

// FTC_SBitCache_Lookup(cache.sbit,&imagetype, GetGlyphIndex(ucs),sbit,anode)
// FT_GlyphSlot glyph = GetGlyph(ucs,
// 	FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL, face);
// FT_Bitmap bitmap = glyph->bitmap;
// bx = glyph->bitmap_left;
// by = glyph->bitmap_top;
// width = bitmap.width;
// height = bitmap.rows;
// advance = glyph->advance.x >> 6;
// buffer = bitmap.buffer;

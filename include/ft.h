#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_CACHE_H

typedef struct {
  FT_Face face;
  FT_Library library;
  FTC_Manager manager;
} FT_Freeables;

FT_Freeables typesetter();
void free_ft(FT_Freeables f);
int renderer(FT_Face face);
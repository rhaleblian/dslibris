//Gfx converted using Mollusk's PAGfx Converter

//This file contains all the .h, for easier inclusion in a project

#ifndef ALL_GFX_H
#define ALL_GFX_H

#ifndef PAGfx_struct
    typedef struct{
    void *Map;
    int MapSize;
    void *Tiles;
    int TileSize;
    void *Palette;
    int *Info;
} PAGfx_struct;
#endif


// Background files : 
extern const int burl_Info[3]; // BgMode, Width, Height
extern const unsigned short burl_Map[768] __attribute__ ((aligned (4))) ;  // Pal : burl_Pal
extern const unsigned char burl_Tiles[13824] __attribute__ ((aligned (4))) ;  // Pal : burl_Pal
extern PAGfx_struct burl; // background pointer


// Palette files : 
extern const unsigned short burl_Pal[218] __attribute__ ((aligned (4))) ;


#endif


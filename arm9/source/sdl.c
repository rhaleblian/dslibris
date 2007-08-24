#include "SDL/SDL.h"
#include <stdlib.h>

void ShowBMP(char *file, SDL_Surface *screen, int x, int y)
{
    SDL_Surface *image;
    SDL_Rect dest;

    /* Load the BMP file into a surface */
    image = SDL_LoadBMP(file);
    if ( image == NULL ) {
        fprintf(stderr, "Couldn't load %s: %s\n", file, SDL_GetError());
        return;
    }

    /* Blit onto the screen surface.
       The surfaces should not be locked at this point.
     */
    dest.x = x;
    dest.y = y;
    dest.w = image->w;
    dest.h = image->h;
    SDL_BlitSurface(image, NULL, screen, &dest);

    /* Update the changed portion of the screen */
    SDL_UpdateRects(screen, 1, &dest);
}

int sdlInit(void) {
  if ( SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0 ) {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    return(1);
  }
  atexit(SDL_Quit);

  SDL_Surface *screen;
  
  screen = SDL_SetVideoMode(256, 192, 24, SDL_SWSURFACE);
  if ( screen == NULL ) {
    fprintf(stderr, "Unable to set video: %s\n", SDL_GetError());
    return(1);
  }
  
  ShowBMP("burl.bmp",screen,(256-192)/2,0);
  return(0);
}

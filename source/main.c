#include <nds.h>
#include <PA9.h>
#include <stdio.h>
#include <sys/dir.h>
#include <libfat.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "gfx/all_gfx.c"
#include "gfx/all_gfx.h"

#define FT_FLOOR(x)     (((x) & -64) / 64)
#define FT_CEIL(x)      ((((x) + 63) & -64) / 64)
#define FT_FIXED(x)     ((x) * 64) 
u16 *ft_buffer = BG_GFX;
u16 ft_buffer_w = SCREEN_WIDTH;
u16 ft_buffer_h = SCREEN_HEIGHT;
u32 ft_pen_r = 0;
u32 ft_pen_g = 0;
u32 ft_pen_b = 0; 

#define HEADER 8
#define MARGIN 16
#define HEIGHT 192
#define WIDTH 256
#define ROTATERH 0
#define ROTATELH 1
#define STARTFONT 5
#define MAXDRAWCHARS 2048
#define BUFSIZE 128000

uint16 font = STARTFONT;
uint16 rotation = ROTATERH;
uint16 charstart = 0;
uint16 charend = 0;
uint16 charlast = 0;
uint16 displayleft = 1;
uint16 displayright = 0;
char buf[BUFSIZE] = "";

int ftdemo(void) {
// INIT FreeType2
    FT_Library library;
    FT_Error   error;
    FT_Face    face;
	FT_Bitmap  *bitmap;

    error = FT_Init_FreeType(&library);
    if(error)
        return error;

    error = FT_New_Face(library, "fruitiger.ttf", 0, &face);
    if(error)
        return error;

   FT_Set_Pixel_Sizes(face, 0, 14);


// RENDER Text
    const char *string = "Hello universe!";
    int x = 3;
    int y = 3; // at [3, 3]
   
    int ascent = FT_CEIL(FT_MulFix((face->bbox).yMax, (face->size->metrics).y_scale));
    y += ascent;      // add ascent to y coordinate

    int i;
    for(i=0; string[i] != 0; i++)
    {
        if(string[i] == ' ')
        {
           x += ascent/2;
            continue;
        }
        else if(string[i] == '\n')
        {
            y += ascent;
            continue;
        }
        else
        {
           FT_Load_Char(face, string[i], FT_LOAD_RENDER | // tell FreeType2 to render internally
                                           FT_LOAD_TARGET_NORMAL); // grayscale 0-255
           
            u8 *src = (u8 *)&(face->glyph->bitmap);
            u16 *dest = BG_GFX;

            int j, k;
            u16 *pixel, r, g, b;

            if(bitmap->pitch >= 0)
            {
                for(j=0; j<bitmap->rows; j++)
                {
                    for(k=0; k<bitmap->width; k++)
                    {
                        u8 alpha = src[k]; // 0-255
                        if(alpha)
                        {
                            pixel = &dest[j*ft_buffer_w + k];   // original pixel
                           
                            r = ((*pixel)&31)*(255-alpha)     + 255*alpha; // blend algorithm, our color is red
                            g = ((*pixel>>5)&31)*(255-alpha)  + 0*alpha;
                            b = ((*pixel>>10)&31)*(255-alpha) + 0*alpha;
                           
                            r = (r>>8) & 31; // convert to R5G5B5
                            g = (g>>8) & 31;
                            b = (b>>8) & 31;
                            *pixel = BIT(15) | RGB15(r,g,b); // write pixel
                        }
                    }
 //                   src += pitch;
                }
            }

           x += FT_CEIL(face->glyph->metrics.width) + 1; // add incorrect but cheap letterspacing
       }
    } 
	return 0;
}

void init(void) {
	PA_Init();
	PA_InitVBL();
	PA_SetVideoMode(0,1);  // one rotating background
	PA_SetVideoMode(1,1);  // one rotating background
	PA_InitText(0,3);
	PA_InitText(1,3);
	PA_Init8bitBg(0,3);
	PA_Init8bitBg(1,3);
	PA_SetBgPalCol(0,0,PA_RGB(31,31,31));
	PA_SetBgPalCol(0,1,PA_RGB(0,0,0));
	PA_SetBgPalCol(1,0,PA_RGB(31,31,31));
	PA_SetBgPalCol(1,1,PA_RGB(0,0,0));
	PA_8bitCustomFont(5, fruitiger);
	PA_8bitCustomFont(6, lucidasans);
	PA_8bitCustomFont(7, bookantiqua);
}

void clearpage(uint16 display) {
	PA_SmartText(display,0,0,256,192,"",0,font,0,MAXDRAWCHARS);
}

s16 layoutpage(uint16 display, char* text) {
	return PA_SmartText(display,
		MARGIN,HEADER,
		HEIGHT-MARGIN,WIDTH-HEADER,
		text,1,font,rotation+3,MAXDRAWCHARS);
}

s16 layoutpageinvisible(uint16 display, char* text) {
	return PA_SmartText(display,
		MARGIN,HEADER,
		HEIGHT-MARGIN,WIDTH-HEADER,
		text,2,font,2,MAXDRAWCHARS);
}

void layoutpages(void) {
	charend = charstart;
	clearpage(displayleft);
	charend += layoutpage(displayleft,&(buf[charend]));
	clearpage(displayright);
	if(charend < strlen(buf)) {
		charend += layoutpage(displayright,&(buf[charend]));
	}
}

uint16 pageback(void) {
	char reverse[MAXDRAWCHARS];

	// walk charstart backwards two pages.	
	if(charstart == 0) return -1;
	
	int i;
	for(i=0;(i < MAXDRAWCHARS) && (charstart-i >= 0); i++) {
		reverse[i] = buf[charstart-i];
	}
	uint16 count = layoutpageinvisible(displayright,reverse);
	charstart -=count;
	
	if(charstart <= 0) {
		charstart = 0;
		return -2;
	}
	
	for(i=0;(i < MAXDRAWCHARS) && (charstart-i >= 0); i++) {
		reverse[i] = buf[charstart-i];
	}
	count = layoutpageinvisible(displayleft,reverse);
	charstart -=count;

	return 0;
}

void splash(void) {
}

int main(void) {
	init();	
	
	if(fatInitDefault())
		strcat(buf,"[fatlib inits.]\n");
	else
		strcat(buf,"[fatlib init fails.]\n");
	
	FILE *fp = fopen("/ebook.txt","r");
	if(!fp) {
		strcat(buf,"[ebook.txt cannot be opened.]\n");
	} else {
		strcat(buf,"[opened ebook.txt.]\n");
		fread(buf, sizeof(char), BUFSIZE, fp);
	}
	fclose(fp);

	layoutpages();
	while(1) {
		if(Pad.Newpress.R) {
			// page forward
			if(charend < strlen(buf)) {
				charstart = charend;
				layoutpages();
			}
		}
		if(Pad.Newpress.L) {
			// page backward
			if(charstart > 0) {
				pageback();
				layoutpages();
			}
		}
		if(Pad.Newpress.B) {
			// go to page 1
			charstart=0;
			layoutpages();
		}
		if(Pad.Newpress.X) {
			// rotate
			rotation = !rotation;
			displayleft = !displayleft;
			displayright = !displayright;
			layoutpages();
		}
		if(Pad.Newpress.Y) {
			font++;
			font %= 7;
			layoutpages();
		}
		PA_WaitForVBL();
	}
	return 0;
}

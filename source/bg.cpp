#include <nds.h>
#include <splash.h>
#include <bg.h>

void drawsplash(u16 *screen) {
    // set the mode for 2 text layers and two extended background layers
	videoSetMode(MODE_5_2D);

	// set the sub background up for text display (we could just print to one
	// of the main display text backgrounds just as easily
	videoSetModeSub(MODE_0_2D); //sub bg 0 will be used to print text

	vramSetBankA(VRAM_A_MAIN_BG);

	// set up our bitmap background
	bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0,0);

	decompress(splashBitmap, BG_GFX,  LZ77Vram);
}
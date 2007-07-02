#include <nds.h>
#include <PA9.h>

#define HEADER 8
#define MARGIN 16
#define HEIGHT 192
#define WIDTH 256
#define ROTATERH 0
#define ROTATELH 1

char pageleft[256]="Here's text on the left page. This font looks proportional. Will this string wrap around to the next line?\nThis is the next line.";
char pageright[256]="This text should show up on the right page.";

uint16 fontsize = 3;   // 1-4
uint16 rotation = ROTATERH;

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
}

void clearpage(uint16 display) {
	PA_SmartText(display,0,0,256,192,"",2,fontsize,0,256);
}

void layoutpage(uint16 display, char* text) {
	PA_SmartText(display,
		MARGIN,HEADER,
		HEIGHT-MARGIN,WIDTH-HEADER,
		text,2,fontsize,rotation+3,256);
}

void layoutpages(void) {
	clearpage(0);
	clearpage(1);
	layoutpage(!rotation,pageleft);
	layoutpage(rotation,pageright);
}

void draw(void) {
	PA_WaitForVBL();
}

int main(void) {
	init();
	layoutpages();
	while(1) {
		if(Pad.Newpress.X) {
			rotation = !rotation;
			layoutpages();
		}
		draw();
	}
	return 0;
}

#include <nds.h>
#include <PA9.h>
#include <stdio.h>
#include <sys/dir.h>
#include <libfat.h>

#define HEADER 8
#define MARGIN 16
#define HEIGHT 192
#define WIDTH 256
#define ROTATERH 0
#define ROTATELH 1

#define BUFSIZE 30000
char buf[BUFSIZE] = "";

uint16 fontsize = 3;   // 1-4
uint16 rotation = ROTATERH;
uint16 charstart = 0;
uint16 charend = 0;
uint16 charlast = 0;

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

s16 layoutpage(uint16 display, char* text) {
	return PA_SmartText(display,
		MARGIN,HEADER,
		HEIGHT-MARGIN,WIDTH-HEADER,
		text,2,fontsize,rotation+3,512);
}

void layoutpages(void) {
	charend = charstart;
	if(charend >= strlen(buf)) return;
	clearpage(!rotation);
	charend += layoutpage(!rotation,&(buf[charstart]));
	clearpage(rotation);
	if(charend < strlen(buf)) {
		charend += layoutpage(rotation,&(buf[charend]));
	}
}

int main(void) {
	init();	
	
	if(fatInitDefault())
		strcat(buf,"[fatlib inits.]\n");
	else
		strcat(buf,"[fatlib failed.]\n");
	
	FILE *fp = fopen("/ebook.txt","r");
	if(!fp) {
		strcat(buf,"[ebook.txt cannot be opened.]\n");
	} else {
		strcat(buf,"[ebook.txt opened.]\n");
		fread(buf, sizeof(char), BUFSIZE, fp);
	}
	fclose(fp);

	layoutpages();
	while(1) {
		if(Pad.Newpress.X) {
			rotation = !rotation;
			layoutpages();
		}
		if(Pad.Newpress.R) {
			if(charend < strlen(buf)) {
				charstart = charend;
				layoutpages();
			}
		}
		if(Pad.Newpress.Y) {
			charstart=0;
			layoutpages();
		}
		PA_WaitForVBL();
	}
	return 0;
}

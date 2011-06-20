#ifndef SCREEN_H
#define SCREEN_H

#include <nds.h>
#include <unistd.h>

class Screen
{
	u16* frontBuffer, backBuffer;
	u16 colorMask;

public:
	Screen();
	u16* GetBuffer();
	void SwapBuffers();
}

#endif


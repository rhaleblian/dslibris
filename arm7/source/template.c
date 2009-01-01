/*------------------------------------------------------------------------------

default ARM7 core

Copyright (C) 2005
Michael Noland (joat)
Jason Rogers (dovoto)
Dave Murphy (WinterMute)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.
2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.
3.	This notice may not be removed or altered from any source
distribution.

------------------------------------------------------------------------------*/
#include <nds.h>
#include <dswifi7.h>
#include <maxmod7.h>
#include "ndsx_brightness.h"

#define SET_BRIGHTNESS_NEXT     (0x1211B215) // On Phatty DS: SET_BACKLIGHTS_TOGGLE
#define PM_DSLITE_REG   (4)
extern void powerValueHandler(u32 value, void* user_data);

//------------------------------------------------------------------------------
void VcountHandler() {
//------------------------------------------------------------------------------
	inputGetAndSend();
}

//------------------------------------------------------------------------------
void VblankHandler(void) {
//------------------------------------------------------------------------------
	Wifi_Update();
}

void extendedHandler(u32 value, void* user_data)
{
	switch (value)
	{
/*		case SET_BRIGHTNESS_NEXT:
			s32 nextlevel = 
			readPowerManagement(PM_DSLITE_REG) - 64 + 1;
			if(nextlevel > 3) 
				nextlevel = 0;
			writePowerManagement(PM_DSLITE_REG, nextlevel);
			return;
		case SET_BRIGHTNESS_0:*/
		case SET_BRIGHTNESS_NEXT:
		case SET_BRIGHTNESS_0:
		case SET_BRIGHTNESS_1:
		case SET_BRIGHTNESS_2:
		case SET_BRIGHTNESS_3:
			NDSX_BrightnessFifo(value);
			break;
		default:
			powerValueHandler(value, user_data);
	}
}

//------------------------------------------------------------------------------
int main() {
//------------------------------------------------------------------------------
	irqInit();
	fifoInit();

	// read User Settings from firmware
	readUserSettings();

	// Start the RTC tracking IRQ
	initClockIRQ();

	SetYtrigger(80);

	installWifiFIFO();
	installSoundFIFO();

	mmInstall(FIFO_MAXMOD);

//	installSystemFIFO();
	fifoSetValue32Handler(FIFO_PM, extendedHandler, 0);

	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_VBLANK, VblankHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK);   

	// Keep the ARM7 mostly idle
	while (1) swiWaitForVBlank();
}



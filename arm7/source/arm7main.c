/*---------------------------------------------------------------------------------

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

---------------------------------------------------------------------------------*/
#include <nds.h>
//#include <dswifi7.h>
//#include <maxmod7.h>
#include <nds/fifocommon.h>
#include "ndsx_brightness.h"
//---------------------------------------------------------------------------------
void VcountHandler() {
	//---------------------------------------------------------------------------------
	inputGetAndSend();
}

// FifoValue32HandlerFunc type function
void brightness_fifo(u32 msg, void* data) { //incoming fifo message
	msg%=4;
	if((bool)(readPowerManagement(PM_DSLITE_REG) & PM_IS_LITE))
	{
		writePowerManagement(PM_DSLITE_REG, msg);
	} 
	else // Is Phatty!
	{
		if(msg == 0)
		{
			u32 reg_without_backlight = readPowerManagement(PM_CONTROL_REG) & ~PM_BACKLIGHTS;
			writePowerManagement(PM_CONTROL_REG, reg_without_backlight);
			return;
		}
		else if(msg == 1 ||
			msg == 2 ||
			msg == 3)
		{
			u32 reg_with_backlight = readPowerManagement(PM_CONTROL_REG) | PM_BACKLIGHTS;
			writePowerManagement(PM_CONTROL_REG, reg_with_backlight);
			return;
		}
	} 
}

//---------------------------------------------------------------------------------
int main() {
//---------------------------------------------------------------------------------
	irqInit();
	fifoInit();

	// read User Settings from firmware
	readUserSettings();

	// Start the RTC tracking IRQ
	initClockIRQ();

	SetYtrigger(80);

	//installWifiFIFO();
	//installSoundFIFO();
	//mmInstall(FIFO_MAXMOD);

	installSystemFIFO();

	fifoSetValue32Handler(BACKLIGHT_FIFO,&brightness_fifo,0);

	
	irqSet(IRQ_VCOUNT, VcountHandler);
	//irqSet(IRQ_VBLANK, VblankHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK); 

	// Keep the ARM7 mostly idle
	while (1) swiWaitForVBlank();
}

/*

dslibris - An ebook reader for Nintendo DS.
 
 Copyright (C) 2007 Ray Haleblian
 Portions gratefully stolen from DSGUI, DSFTP, and devkitPro.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include <nds.h>
#include <stdlib.h>
#include <nds/bios.h>
#include <nds/reload.h>
#include <nds/arm7/touch.h>
#include <nds/arm7/clock.h>
#include <dswifi7.h>
#include <DSGUI/BIPCCodes.h>
#include "pm.h"

/**-----------------------------------------------------------------------**/
void startSound(int sampleRate, const void* data, u32 bytes,
				u8 channel, u8 vol,  u8 pan, u8 format) {

	SCHANNEL_TIMER(channel)  = SOUND_FREQ(sampleRate);
	SCHANNEL_SOURCE(channel) = (u32)data;
	SCHANNEL_LENGTH(channel) = bytes >> 2 ;
	SCHANNEL_CR(channel)     = SCHANNEL_ENABLE | SOUND_ONE_SHOT 
		| SOUND_VOL(vol) | SOUND_PAN(pan) | (format==1?SOUND_8BIT:SOUND_16BIT);
}


/**-----------------------------------------------------------------------**/
s32 getFreeSoundChannel()
{
	int i;
	for (i=0; i<16; i++) {
		if ( (SCHANNEL_CR(i) & SCHANNEL_ENABLE) == 0 ) return i;
	}
	return -1;
}

int vcount;
touchPosition first,tempPos;


/**-----------------------------------------------------------------------**/
void KeydownHandler()
{
	VBLANK_INTR_WAIT_FLAGS |= IRQ_KEYS;
}

/**-----------------------------------------------------------------------**/
void VcountHandler()
{  
	static int lastbut = -1;
	
	uint16 but=0, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0;

	but = REG_KEYXY;

	if (!( (but ^ lastbut) & (1<<6))) {

		tempPos = touchReadXY();

		x = tempPos.x;
		y = tempPos.y;
		xpx = tempPos.px;
		ypx = tempPos.py;
		z1 = tempPos.z1;
		z2 = tempPos.z2;
		
	} else {
		lastbut = but;
		but |= (1 <<6);
	}

/*

	if ( vcount == 80 ) {
		first = tempPos;
	} else {
		if (	abs( xpx - first.px) > 10 || abs( ypx - first.py) > 10 ||
			(but & ( 1<<6)) ) {

			but |= (1 <<6);
			lastbut = but;

		} else { 	
			IPC->mailBusy = 1;
*/
			IPC->touchX			= x;
			IPC->touchY			= y;
			IPC->touchXpx		= xpx;
			IPC->touchYpx		= ypx;
			IPC->touchZ1		= z1;
			IPC->touchZ2		= z2;
/*
			IPC->mailBusy = 0;
		}
	}
*/
	IPC->buttons		= but;
//	vcount ^= (80 ^ 130);
//	SetYtrigger(vcount);

	// Check if the lid has been closed.
	if(but & BIT(7)) {

		// Save the current interrupt sate.
		u32 ie_save = REG_IE;

		// Turn the speaker down.
		swiChangeSoundBias(0,0x400);

		// Save current power state.
		int power = readPowerManagement(PM_CONTROL_REG);

		// Set sleep LED.
		writePowerManagement(PM_CONTROL_REG, PM_LED_CONTROL(1));

		// Register for the lid interrupt.
		REG_IE = IRQ_LID;

		// Power down till we get our interrupt.
		swiSleep(); //waits for PM (lid open) interrupt
		REG_IF = ~0;

		// Restore the interrupt state.
		REG_IE = ie_save;

		// Restore power state.
		writePowerManagement(PM_CONTROL_REG, power);

		// Turn the speaker up.
		swiChangeSoundBias(1,0x400);
	}
}


/**-----------------------------------------------------------------------**/
void VblankHandler(void) {

  static int heartbeat = 0;

  uint16 but=0, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0, batt=0, aux=0;
  int t1=0, t2=0;
  uint32 temp=0;
  uint8 ct[sizeof(IPC->time.curtime)];
  u32 i;

  // Update the heartbeat
  heartbeat++;

  // Read the touch screen

  but = REG_KEYXY;

  if (!(but & (1<<6))) {

    touchPosition tempPos = touchReadXY();

    x = tempPos.x;
    y = tempPos.y;
    xpx = tempPos.px;
    ypx = tempPos.py;
  }

  z1 = touchRead(TSC_MEASURE_Z1);
  z2 = touchRead(TSC_MEASURE_Z2);

	
  batt = touchRead(TSC_MEASURE_BATTERY);
  aux  = touchRead(TSC_MEASURE_AUX);

  // Read the time
  rtcGetTime((uint8 *)ct);
  BCDToInteger((uint8 *)&(ct[1]), 7);

  // Read the temperature
  temp = touchReadTemperature(&t1, &t2);

  // Update the IPC struct
  //IPC->heartbeat	= heartbeat;
  IPC->buttons		= but;
  IPC->touchX		= x;
  IPC->touchY		= y;
  IPC->touchXpx		= xpx;
  IPC->touchYpx		= ypx;
  IPC->touchZ1		= z1;
  IPC->touchZ2		= z2;
  IPC->battery		= batt;
  IPC->aux			= aux;

  for(i=0; i<sizeof(ct); i++) {
    IPC->time.curtime[i] = ct[i];
  }

  IPC->temperature = temp;
  IPC->tdiode1 = t1;
  IPC->tdiode2 = t2;


  //sound code  :)
  TransferSound *snd = IPC->soundData;
  IPC->soundData = 0;

  if (0 != snd) {

    for (i=0; i<snd->count; i++) {
      s32 chan = getFreeSoundChannel();

      if (chan >= 0) {
	startSound(snd->data[i].rate, snd->data[i].data, snd->data[i].len, chan, snd->data[i].vol, snd->data[i].pan, snd->data[i].format);
      }
    }
  }

	Wifi_Update();

}

/**-----------------------------------------------------------------------**/
// callback to allow wifi library to notify arm9
void arm7_synctoarm9() { // send fifo message
//  REG_IPC_FIFO_TX = 0x87654321;
  REG_IPC_FIFO_TX = IPC_WIFI_SYNC;

}

/**-----------------------------------------------------------------------**/
// interrupt handler to allow incoming notifications from arm9

void arm7_fifo() { // check incoming fifo messages
	bool top, bottom;
	u32 msg = REG_IPC_FIFO_RX;

	switch(IPC_COMMAND(msg))
    {
		case IPC_WIFI_SYNC:
		Wifi_Sync();
		REG_IPC_FIFO_TX = IPC_OK;
		break;

		case IPC_BACKLIGHT:
		if(IPC_ARG(msg) != IPC_GET)
		pmSwitchBacklight(msg & IPC_BACKLIGHT_TOP,
			msg & IPC_BACKLIGHT_BOTTOM);
		pmGetBacklight(&top, &bottom);
		REG_IPC_FIFO_TX = (IPC_OK |
			(top ? IPC_BACKLIGHT_TOP : 0) |
			(bottom ? IPC_BACKLIGHT_BOTTOM : 0));
		break;
		  
		case IPC_BRIGHTNESS:
		if(IPC_ARG(msg) != IPC_GET)
			pmSetBacklightBrightness(IPC_ARG(msg));		
		REG_IPC_FIFO_TX = IPC_OK | pmGetBacklightBrightness();
		break;
		  
		case IPC_POWEROFF:
		pmPowerOff();
		REG_IPC_FIFO_TX = IPC_OK;

	}
}

/**-----------------------------------------------------------------------**/
int main( void)
{
	// enable & prepare fifo asap
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR; 

	// Reset the clock if needed
	rtcReset();

	//enable sound
	powerON(POWER_SOUND);
	SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7F);
	IPC->soundData = 0;

	irqInit();

	//initClockIRQ();

	irqSet(IRQ_VBLANK, VblankHandler);
	irqEnable(IRQ_VBLANK);

	irqSet(IRQ_WIFI,Wifi_Interrupt);
	irqEnable(IRQ_WIFI);

	{
		// sync with arm9 and init wifi
		u32 fifo_temp;   

		while(1) { // wait for magic number
			while(REG_IPC_FIFO_CR&IPC_FIFO_RECV_EMPTY) swiWaitForVBlank();
			fifo_temp=REG_IPC_FIFO_RX;
			if(fifo_temp==IPC_WIFI_INIT) break;
		}
		while(REG_IPC_FIFO_CR&IPC_FIFO_RECV_EMPTY) swiWaitForVBlank();
		fifo_temp=REG_IPC_FIFO_RX; // give next value to wifi_init
		Wifi_Init(fifo_temp);

		irqSet(IRQ_FIFO_NOT_EMPTY,arm7_fifo); // set up fifo irq
		irqEnable(IRQ_FIFO_NOT_EMPTY);
		REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_RECV_IRQ;

		// allow wifi lib to notify arm9
		Wifi_SetSyncHandler(arm7_synctoarm9);

	} // arm7 wifi init complete

	irqSet(IRQ_VCOUNT, VcountHandler);
	irqEnable(IRQ_VCOUNT);

	// Keep the ARM7 out of main RAM
	while (1) {
		swiWaitForVBlank();

#if 0
		// reboot?
		if((*(vu32*)0x27ffffc) != 0)
		{
			REG_IE = 0;
			REG_IME = 0;
			REG_IF = 0xffff;

			typedef void (*eloop_type)(void);
			eloop_type eloop = *(eloop_type*)0x27ffffc;
			*(vu32*)0x27ffffc = 0;
			*(vu32*)0x27ffff8 = 0;

			eloop();
		}
#endif
	}
}


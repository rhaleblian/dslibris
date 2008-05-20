#include <nds.h>
#include <stdlib.h>
#include <nds/bios.h>
#include <nds/reload.h>
#include <nds/arm7/touch.h>
#include <nds/arm7/clock.h>
#ifdef WIFIDEBUG
#include <dswifi7.h>
#endif
#include "ndsx_brightness.h"

/**-------------------------------------------------------------------------**/
void startSound(int sampleRate, const void* data, u32 bytes,
				u8 channel, u8 vol,  u8 pan, u8 format) {

	SCHANNEL_TIMER(channel)  = SOUND_FREQ(sampleRate);
	SCHANNEL_SOURCE(channel) = (u32)data;
	SCHANNEL_LENGTH(channel) = bytes >> 2 ;
	SCHANNEL_CR(channel)     = SCHANNEL_ENABLE | SOUND_ONE_SHOT 
		| SOUND_VOL(vol) | SOUND_PAN(pan) | (format==1?SOUND_8BIT:SOUND_16BIT);
}


/**-------------------------------------------------------------------------**/
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


/**-------------------------------------------------------------------------**/
void KeydownHandler()
{
	VBLANK_INTR_WAIT_FLAGS |= IRQ_KEYS;
}

/**-------------------------------------------------------------------------**/
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


/**-------------------------------------------------------------------------**/
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
  IPC->touchX			= x;
  IPC->touchY			= y;
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

#ifdef WIFIDEBUG
	Wifi_Update();
#endif

}

/**-------------------------------------------------------------------------**/
// callback to allow wifi library to notify arm9
void arm7_synctoarm9() { // send fifo message
  REG_IPC_FIFO_TX = 0x87654321;
}

/**-------------------------------------------------------------------------**/
// interrupt handler to allow incoming notifications from arm9

#define GET_BRIGHTNESS      (0x1211B210)
#define SET_BRIGHTNESS_0    (0x1211B211)
#define SET_BRIGHTNESS_1    (0x1211B212)
#define SET_BRIGHTNESS_2    (0x1211B213)
#define SET_BRIGHTNESS_3    (0x1211B214)

void arm7_fifo() { // check incoming fifo messages
  int syncd = 0;
  while ( !(REG_IPC_FIFO_CR & IPC_FIFO_RECV_EMPTY)) {
    u32 msg = REG_IPC_FIFO_RX;
    if ( msg == 0x87654321 && !syncd) {
      syncd = 1;
    }
    else if(msg == GET_BRIGHTNESS)
    {
      // send back the value (0 - 3)
      REG_IPC_FIFO_TX = readPowerManagement((4)) - 64;
    }
    else if(msg == SET_BRIGHTNESS_0)
    {
      // write value (0)
      writePowerManagement((4), 0);
    }
    else if(msg == SET_BRIGHTNESS_1)
    {
      // write value (1)
      writePowerManagement((4), 1);
    }
    else if(msg == SET_BRIGHTNESS_2)
    {
      // write value (2)
      writePowerManagement((4), 2);
    }
    else if(msg == SET_BRIGHTNESS_3)
    {
      // write value (3)
      writePowerManagement((4), 3);
    }
  }
}


void FifoHandler()
{
    u32 msg = REG_IPC_FIFO_RX;
    NDSX_BrightnessFifo(msg);
}


/**-------------------------------------------------------------------------**/
int main( void)
{
	// reload
	//LOADNDS->PATH = 0;

	// enable & prepare fifo asap
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR; 

	// Reset the clock if needed
	rtcReset();

	//enable sound
	powerON(POWER_SOUND);
	SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7F);
	IPC->soundData = 0;

	irqInit();

	initClockIRQ();

	irqSet(IRQ_VBLANK, VblankHandler);
	irqEnable(IRQ_VBLANK);

#ifdef WIFIDEBUG
	irqSet(IRQ_WIFI,Wifi_Interrupt);
	irqEnable(IRQ_WIFI);
#endif

	irqSet(IRQ_FIFO_NOT_EMPTY,FifoHandler); // set up fifo irq
	irqEnable(IRQ_FIFO_NOT_EMPTY);
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_RECV_IRQ;

	irqSet(IRQ_VCOUNT, VcountHandler);
	irqEnable(IRQ_VCOUNT);
/*
	irqSet(IRQ_KEYS, KeydownHandler);
	irqEnable(IRQ_KEYS);
*/	
	// Keep the ARM7 out of main RAM
	while (1) swiWaitForVBlank();
}


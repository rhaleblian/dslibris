#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <dswifi9.h>
#include "wifi.h"

void arm9_fifo() {}
void arm9_synctoarm7() {}
void arm9_timer() { Wifi_Timer(50); }

void wifiInit(void) {  
  {
    // send fifo message to initialize the arm7 wifi
    // enable & clear FIFO
    REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
		
    u32 Wifi_pass= Wifi_Init(WIFIINIT_OPTION_USELED);
    REG_IPC_FIFO_TX=0x12345678;
    REG_IPC_FIFO_TX=Wifi_pass;
   	
    *((volatile u16 *)0x0400010E) = 0; // disable timer3
		
    //    irqInit(); 
    irqSet(IRQ_TIMER3, arm9_timer); // setup timer IRQ
    irqEnable(IRQ_TIMER3);
    irqSet(IRQ_FIFO_NOT_EMPTY, arm9_fifo); // setup fifo IRQ
    irqEnable(IRQ_FIFO_NOT_EMPTY);

    iprintf("Timers enabled\n");

    // enable FIFO IRQ
    REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_RECV_IRQ;
   	
    Wifi_SetSyncHandler(arm9_synctoarm7); 
    // tell wifi lib to use our handler to notify arm7

    // set timer3
    *((volatile u16 *)0x0400010C) = -6553; // 6553.1 * 256 cycles = ~50ms;
    *((volatile u16 *)0x0400010E) = 0x00C2; // enable, irq, 1/256 clock
		
    while(Wifi_CheckInit()==0) { // wait for arm7 to be initted successfully
      //while(VCOUNT>192); // wait for vblank
      swiWaitForVBlank();
    }
		
  } // wifi init complete - wifi lib can now be used!
	
  iprintf("Connecting via WFC data\n");
  { // simple WFC connect:
    int i;
    Wifi_AutoConnect(); // request connect
    while(1) {
      i=Wifi_AssocStatus(); // check status
      if(i==ASSOCSTATUS_ASSOCIATED) {
	iprintf("Connected successfully!\n");
	break;
      }
      if(i==ASSOCSTATUS_CANNOTCONNECT) {
	iprintf("Could not connect!\n");
	while(1);
	break;
      }
    } 
    // if connected, you can now use the berkley sockets interface
    // to connect to the internet!
  }
}

#if 0
void wifiMain_wifiChat(){
	IPC->mailData=0;
	IPC->mailSize=0;
	wifiOn=false;
	powerON(POWER_ALL_2D);
	u32 Wifi_pass= Wifi_Init(0);
	IPC->mailData=2;
	while(IPC->mailSize!=2)	IPC->mailData=2;
	IPC->mailData=Wifi_pass;
/*
	*((volatile u16 *)0x0400010E) = 0;
	irqInit();
	irqSet(IRQ_TIMER3, Wifi_Timer_50ms);
	irqEnable(IRQ_TIMER3);
	*((volatile u16 *)0x0400010C) = -13106; // 13107.2 * 256 cycles = ~50ms;
	*((volatile u16 *)0x0400010E) = 0x00C2; // enable, irq, 1/256 clock
*/
	/*** MODIFICATION *///(*(vuint16*)0x0400010E)
	TIMER3_CR = 0;
	irqSet(IRQ_TIMER3, sgIP_ARP_Timer100ms); // Wifi_Timer_50ms);
	irqEnable(IRQ_TIMER3);
	TIMER3_DATA = (u16)-13106;
	TIMER3_CR = TIMER_IRQ_REQ | TIMER_DIV_256;

	irqSet(IRQ_IPC_SYNC, Wifi_Update);
	irqEnable(IRQ_IPC_SYNC);
	REG_IPC_SYNC = IPC_SYNC_IRQ_ENABLE;

	/**** end */
	scanning=false;

}

#endif


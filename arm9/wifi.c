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
    REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR; // enable & clear FIFO
		
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

    REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_RECV_IRQ; // enable FIFO IRQ
   	
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


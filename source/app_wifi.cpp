#if 0
#include <app.h>
#include <dswifi9.h>
#include <DSGUI/BIPCCodes.h>


#define WIFI_UPDATE_FREQ 50

// notification function to send fifo message to arm7
static void myWifiSyncHandler()
{ // send fifo message
   REG_IPC_FIFO_TX=IPC_WIFI_SYNC;
}

// interrupt handler to receive fifo messages from arm7
static void myFifoIrq()
{ // check incoming fifo messages
   u32 value = REG_IPC_FIFO_RX;
   if(value == IPC_WIFI_SYNC) Wifi_Sync();
}

// wifi timer function
static void myWifiTimer(void)
{
  Wifi_Timer(WIFI_UPDATE_FREQ);
}

void App::WifiInit()
{
  // send fifo message to initialize the arm7 wifi
  REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;

  // acquire magic pass number and transmit it to arm7
  u32 Wifi_pass = Wifi_Init(WIFIINIT_OPTION_USELED);
  REG_IPC_FIFO_TX = IPC_WIFI_INIT;
  REG_IPC_FIFO_TX = Wifi_pass;

  TIMER_CR(3) = 0; // disable timer 3 (fow now)
    
  irqSet(IRQ_TIMER3, myWifiTimer); // setup timer IRQ
  irqEnable(IRQ_TIMER3);
  
  irqSet(IRQ_FIFO_NOT_EMPTY, myFifoIrq); // setup fifo IRQ
  irqEnable(IRQ_FIFO_NOT_EMPTY);
  
  REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_RECV_IRQ; // enable FIFO IRQ
    
  // tell wifi lib to use our handler to notify arm7
  Wifi_SetSyncHandler(myWifiSyncHandler); 
    
  // set timer3
  TIMER_DATA(3) = TIMER_FREQ_256(WIFI_UPDATE_FREQ);
  TIMER_CR(3) = TIMER_ENABLE | TIMER_IRQ_REQ | TIMER_DIV_256;
    
  // wait for arm7 to be initted successfully
  while(Wifi_CheckInit()==0)
  { 
    while(REG_VCOUNT>192); // wait for vblank
    while(REG_VCOUNT<192);
  }
    
  irqSet(IRQ_IPC_SYNC, Wifi_Update);
  irqEnable(IRQ_IPC_SYNC);
  REG_IPC_SYNC = IPC_SYNC_IRQ_ENABLE; 	
}

bool App::WifiConnect()
{
  // simple WFC connect:
  int i;
  Wifi_AutoConnect(); // request connect
 	while(1)
	{    
		while(REG_VCOUNT>192); // wait for vblank
		while(REG_VCOUNT<192);

	    i = Wifi_AssocStatus(); // check status

		switch(i)
		{
			case ASSOCSTATUS_SEARCHING:
			Log("info : searching for access point.\n");
			PrintStatus("[searching for AP]");
			break;
			  
			case ASSOCSTATUS_AUTHENTICATING:
			Log("info : authenticating.\n");
			PrintStatus("[authenticating]");
			break;
			  
			case ASSOCSTATUS_ASSOCIATING:
			Log("progr: associating.\n");
			PrintStatus("[associating]");
			break;
			  
			case ASSOCSTATUS_ACQUIRINGDHCP:
			Log("progr: acquiring DHCP.\n");
			PrintStatus("[acquiring DHCP]");
			break;

			case ASSOCSTATUS_ASSOCIATED:
			Log("progr: connected.\n");
			PrintStatus("[connected]");
			return true;
			  
			case ASSOCSTATUS_CANNOTCONNECT:
			Log("warn : could not connect.\n");
			PrintStatus("[cannot connect]");
			Wifi_DisconnectAP();
			return false;
			  
			default:
			break;
		}
	}
	return false;
}

#endif

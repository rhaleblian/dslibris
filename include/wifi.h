/*
* 
*   DSWifi Chat - Copyright 2006 Bafio
*   This software is completely free. No warranty is provided.
*   If you use it, please give me credit and email me about your
*   project at buzurro@gmail.com
*   Contributions are appriciated too.
*
*/

#pragma once

#if 0

#include "nds.h"

#define WFTIMEOUT 60
#define  RETRIES 3
#define  RETRYDELAY 120 // 2 seconds

typedef struct wifid {
  char ssid[3][32];
  char wepKey[3][13];
  unsigned long ipAddress[3];
  unsigned long  gateway[3];
  unsigned long  dns1[3];
  unsigned long  dns2[3];
  unsigned char subnetLength[3];
  int wepMode[3];
  bool read;
  unsigned char mac[6];
  } wifidata;


/* Address of the shared CommandControl structure */
#define wifiData ((wifidata*)((uint32)(IPC) + sizeof(TransferRegion)))
//#define timeData ((timedata*)((uint32)(wifiData) + sizeof(wifidata)))

char rcvdbuf[4096];
int rcvdlen;
char sendbuf[4096];


char global_ssid[34];
u32 global_ipaddr, global_snmask, global_gateway, global_dns1,global_dns2,global_server;


int global_dhcp;
unsigned char global_wepkeys[4][32];
int global_wepkeyid, global_wepmode;


Wifi_AccessPoint global_connectAP; 

bool wifiOn;
void startScanning();
void wifiMain();

bool connectToAp(bool useDiskConf);

bool readConf();
void writeConf();
int openSocketRandom();
int openSocket(int port);

u32 getGateway();
u32 getIPAddress();

void cleanUp(int so);
int recAndAck(int so,struct sockaddr * addrIn, int timeout);
int rec(int so,struct sockaddr * addrIn, int timeout);
int sen(int so, u32 dest, unsigned short port, int len);
int senOff(int so, u32 dest, unsigned short port, int len, int off);
bool senWaitAck(int so, u32 dest, unsigned short port, int len);
bool senOffWaitAck(int so, u32 dest, unsigned short port, int len, int off);

unsigned long dnsQuery(char* name);

#endif

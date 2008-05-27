// BIPCCodes.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BIPCCODES_H
#define BIPCCODES_H

/* system includes */
/* (none) */

/* my includes */
/* (none) */

#define IPC_COMMAND(x) (x & 0xffffff00)
#define IPC_ARG(x) (x & 0x000000ff)

#define IPC_GET 0x80

#define IPC_WIFI_INIT 0x12345600
#define IPC_WIFI_SYNC 0x87654300

#define IPC_BACKLIGHT 0xbadeaf00
#define IPC_BACKLIGHT_TOP    0x1
#define IPC_BACKLIGHT_BOTTOM 0x2

#define IPC_BRIGHTNESS 0xbabefa00
#define IPC_BRIGHTNESS_0      0x0
#define IPC_BRIGHTNESS_1      0x1
#define IPC_BRIGHTNESS_2      0x2
#define IPC_BRIGHTNESS_3      0x3

#define IPC_POWEROFF  0xdeadba00

#define IPC_OK    0x80000000
#define IPC_ERROR 0x00000000

bool readFifo(u32* val);

#endif /* BIPCCODES_H */

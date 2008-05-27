// BEvent.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BEVENT_H
#define BEVENT_H

/* system includes */
/* (none) */

/* my includes */
#include "BGraphics.h"

typedef enum {
  EVT_KEYMASK   = 0x0100,       
  EVT_KEYDOWN   = 0x0101,
  EVT_KEYHELD   = 0x0110,
  EVT_KEYUP     = 0x0111,
  EVT_TOUCHMASK = 0x1000,
  EVT_TOUCHDOWN = 0x1001,
  EVT_TOUCHDRAG = 0x1010,
  EVT_TOUCHUP   = 0x1011
} BEventType;

typedef struct {
  BEventType type; // EVT_KEYDOWN, EVT_KEYHELD, EVT_KEYUP
  uint32 key;
  uint32 keysHeld;
} BKeyEvent;

typedef struct {
  BEventType type; // EVT_TOUCHDOWN, EVT_TOUCHDRAG, EVT_TOUCHUP
  int x, y;
} BTouchEvent;

typedef union {
  BEventType type;
  BKeyEvent key;
  BTouchEvent touch;
} BEvent;

#endif /* BEVENT_H */

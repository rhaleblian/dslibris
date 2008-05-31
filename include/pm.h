/* This is -*- C -*-)

   pm.h   
   \author: Bjoern Giesler <bjoern@giesler.de>
   
   
   
   $Author: giesler $
   $Locker$
   $Revision$
   $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $
 */

#ifndef PM_H
#define PM_H

/* system includes */
#include <nds.h>

/* my includes */
/* (none) */

void pmSwitchBacklight(bool top, bool bottom);
void pmGetBacklight(bool* top, bool* bottom);

u8 pmGetBacklightBrightness();
void pmSetBacklightBrightness(u8 brightness);

void pmPowerOff();

#endif /* PM_H */

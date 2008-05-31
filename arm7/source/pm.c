/* This is -*- C -*-)

   pm.c   
   \author: Bjoern Giesler <bjoern@giesler.de>
   
   
   
   $Author: giesler $
   $Locker$
   $Revision$
   $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $
 */

/* system includes */
/* (none) */

/* my includes */
#include "pm.h"

static u8 pmGetRegister(int reg)
{
  SerialWaitBusy();
  
  REG_SPICNT = (SPI_ENABLE | SPI_DEVICE_POWER |
		SPI_BAUD_1MHz | SPI_CONTINUOUS);
  REG_SPIDATA = reg | 0x80;
  
  SerialWaitBusy();
  
  REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER |SPI_BAUD_1MHz ;
  REG_SPIDATA = 0;
  
  SerialWaitBusy();
  
  return REG_SPIDATA & 0xff;
}

void static pmSetRegister(int reg, int control)
{
  SerialWaitBusy();
  
  REG_SPICNT = (SPI_ENABLE | SPI_DEVICE_POWER |
		SPI_BAUD_1MHz | SPI_CONTINUOUS);
  REG_SPIDATA = reg;
  
  SerialWaitBusy();
  
  REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER | SPI_BAUD_1MHz;
  REG_SPIDATA = control;
}

void pmSwitchBacklight(bool top, bool bottom)
{
  int control = ((top ? PM_BACKLIGHT_TOP : 0) |
		 (bottom ? PM_BACKLIGHT_BOTTOM : 0));
  control |= (pmGetRegister(0) &
	      ~(PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM));
  pmSetRegister(0, control & 0xff);
}

void pmGetBacklight(bool* top, bool* bottom)
{
  int control = pmGetRegister(0);
  if(control & PM_BACKLIGHT_TOP) *top = true;
  else *top = false;
  if(control & PM_BACKLIGHT_BOTTOM) *bottom = true;
  else *bottom = false;
}
  
u8 pmGetBacklightBrightness()
{
  return readPowerManagement((4)) - 64;
}

void pmSetBacklightBrightness(u8 brightness)
{
  if(brightness > 3) brightness = 3;
  writePowerManagement((4), brightness);
}

void pmPowerOff()
{
  pmSetRegister(0, pmGetRegister(0) | PM_POWER_DOWN);
}

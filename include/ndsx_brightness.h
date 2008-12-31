/**********************************
  Copyright (C) Rick Wong (Lick)
  http://licklick.wordpress.com/
***********************************/
#ifndef _NDSX_BRIGHTNESS_
#define _NDSX_BRIGHTNESS_


#include <nds.h>


/*
    PowerManagement bits that libnds doesn't define (yet?).
    Check out "arm7/serial.h".
*/
#define PM_DSLITE_REG   (4)
#define PM_IS_LITE      BIT(6)
#define PM_BACKLIGHTS   (PM_BACKLIGHT_BOTTOM | PM_BACKLIGHT_TOP)

/*
    FIFO Message-IDs, shared between ARM7 and ARM9.
*/
#define ISLITE_QUERY            (0x1211B207)

#define GET_BRIGHTNESS          (0x1211B210) // On Phatty DS: GET_BACKLIGHTS
#define SET_BRIGHTNESS_0        (0x1211B211) // On Phatty DS: SET_BACKLIGHTS_OFF
#define SET_BRIGHTNESS_1        (0x1211B212) // On Phatty DS: SET_BACKLIGHTS_ON
#define SET_BRIGHTNESS_2        (0x1211B213) // On Phatty DS: SET_BACKLIGHTS_ON
#define SET_BRIGHTNESS_3        (0x1211B214) // On Phatty DS: SET_BACKLIGHTS_ON 
#define SET_BRIGHTNESS_NEXT     (0x1211B215) // On Phatty DS: SET_BACKLIGHTS_TOGGLE
#define SET_BRIGHTNESS_PREVIOUS (0x1211B216) // On Phatty DS: SET_BACKLIGHTS_TOGGLE

#define GET_BACKLIGHTS          (0x1211B217)
#define SET_BACKLIGHTS_ON       (0x1211B218)
#define SET_BACKLIGHTS_OFF      (0x1211B219)
#define SET_BACKLIGHTS_TOGGLE   (0x1211B220)


/*
    ARM9 functions to command the ARM7.

    NOTES:
  - swiWaitForVBlank() makes sure you don't interact with the ARM7
    while it's doing the writePM() and readPM() calls. (Those are
    SPI accessing functions and must run without being interrupted.)
  - When calling SPI-functions like writePM()/readPM(), you ACTUALLY
    REALLY have to DISABLE INTERRUPTS first. But swiWaitForVBlank()
    will suffice most of the times.
*/
#ifdef ARM9
inline bool NDSX_IsLite()
{
    REG_IPC_FIFO_TX = ISLITE_QUERY;
    swiWaitForVBlank();
    while(REG_IPC_FIFO_CR & IPC_FIFO_RECV_EMPTY); 
    return (bool)REG_IPC_FIFO_RX;
}

inline u32  NDSX_GetBrightness()
{
    REG_IPC_FIFO_TX = GET_BRIGHTNESS;
    swiWaitForVBlank();
    while(REG_IPC_FIFO_CR & IPC_FIFO_RECV_EMPTY); 
    return (u32)REG_IPC_FIFO_RX;
}

inline void NDSX_SetBrightness_0()
{ 
    REG_IPC_FIFO_TX = SET_BRIGHTNESS_0;
    swiWaitForVBlank();
}

inline void NDSX_SetBrightness_1()
{ 
    REG_IPC_FIFO_TX = SET_BRIGHTNESS_1; 
    swiWaitForVBlank();
}

inline void NDSX_SetBrightness_2()
{ 
    REG_IPC_FIFO_TX = SET_BRIGHTNESS_2; 
    swiWaitForVBlank();
}

inline void NDSX_SetBrightness_3()
{ 
    REG_IPC_FIFO_TX = SET_BRIGHTNESS_3; 
    swiWaitForVBlank();
}

inline void NDSX_SetBrightness_Next()
{ 
    REG_IPC_FIFO_TX = SET_BRIGHTNESS_NEXT;
    swiWaitForVBlank();
}

inline void NDSX_SetBrightness_Previous()
{ 
    REG_IPC_FIFO_TX = SET_BRIGHTNESS_PREVIOUS;
    swiWaitForVBlank();
}

inline u32  NDSX_GetBacklights()
{
    REG_IPC_FIFO_TX = GET_BACKLIGHTS;
    swiWaitForVBlank();
    while(REG_IPC_FIFO_CR & IPC_FIFO_RECV_EMPTY); 
    return (u32)REG_IPC_FIFO_RX;
}

inline void NDSX_SetBacklights_On()
{ 
    REG_IPC_FIFO_TX = SET_BACKLIGHTS_ON;
    swiWaitForVBlank();
}

inline void NDSX_SetBacklights_Off()
{ 
    REG_IPC_FIFO_TX = SET_BACKLIGHTS_OFF;
    swiWaitForVBlank();
}

inline void NDSX_SetBacklights_Toggle()
{ 
    REG_IPC_FIFO_TX = SET_BACKLIGHTS_TOGGLE;
    swiWaitForVBlank();
}
#endif // ARM9


/*
    ARM7 code.

    NOTES:
  - The BrightnessFifo should be called from an 
    ARM7-fifo-on-recv-interrupt-handler.
*/
#ifdef ARM7
inline void NDSX_BrightnessFifo(u32 msg)
{
    if(msg == ISLITE_QUERY)
    {
        REG_IPC_FIFO_TX = (bool)(readPowerManagement(PM_DSLITE_REG) & PM_IS_LITE);
        return;
    }

    // Is Lite?
    if((bool)(readPowerManagement(PM_DSLITE_REG) & PM_IS_LITE))
    {
        if(msg == GET_BRIGHTNESS)
        {
            REG_IPC_FIFO_TX = readPowerManagement(PM_DSLITE_REG) - 64;
            return;
        }    
        else if(msg == SET_BRIGHTNESS_0)
        {
            writePowerManagement(PM_DSLITE_REG, 0);
            return;
        }
        else if(msg == SET_BRIGHTNESS_1)
        {
            writePowerManagement(PM_DSLITE_REG, 1);
            return;
        }
        else if(msg == SET_BRIGHTNESS_2)
        {
            writePowerManagement(PM_DSLITE_REG, 2);
            return;
        }
        else if(msg == SET_BRIGHTNESS_3)
        {
            writePowerManagement(PM_DSLITE_REG, 3);
            return;
        }
        else if(msg == SET_BRIGHTNESS_NEXT)
        {
            s32 nextlevel = 
                readPowerManagement(PM_DSLITE_REG) - 64 + 1;
            if(nextlevel > 3) 
                nextlevel = 0;
            writePowerManagement(PM_DSLITE_REG, nextlevel);
            return;
        }
        else if(msg == SET_BRIGHTNESS_PREVIOUS)
        {
            s32 previouslevel = 
                readPowerManagement(PM_DSLITE_REG) - 64 - 1;
            if(previouslevel < 0)
                previouslevel = 3;
            writePowerManagement(PM_DSLITE_REG, previouslevel);
            return;
        }
        else if(msg == GET_BACKLIGHTS)
        {
            u32 backlight_bits = readPowerManagement(PM_CONTROL_REG) & PM_BACKLIGHTS;
            if(backlight_bits == 12)
                REG_IPC_FIFO_TX = 2;
            else if(backlight_bits == 8 || backlight_bits == 4)
                REG_IPC_FIFO_TX = 1;
            else
                REG_IPC_FIFO_TX = 0;
            return;
        }
        else if(msg == SET_BACKLIGHTS_ON)
        {
            u32 reg_with_backlight = readPowerManagement(PM_CONTROL_REG) | PM_BACKLIGHTS;
            writePowerManagement(PM_CONTROL_REG, reg_with_backlight);
            return;
        }
        else if(msg == SET_BACKLIGHTS_OFF)
        {
            u32 reg_without_backlight = readPowerManagement(PM_CONTROL_REG) & ~PM_BACKLIGHTS;
            writePowerManagement(PM_CONTROL_REG, reg_without_backlight & 255);
            return;
        }
        else if(msg == SET_BACKLIGHTS_TOGGLE)
        {
            u32 oldreg = readPowerManagement(PM_CONTROL_REG);
            if(oldreg & PM_BACKLIGHTS) 
                oldreg &= ~PM_BACKLIGHTS;
            else
                oldreg |= PM_BACKLIGHTS;
            writePowerManagement(PM_CONTROL_REG, oldreg);
            return;
        }
    } 
    else // Is Phatty!
    {
        if(msg == GET_BRIGHTNESS || 
            msg == GET_BACKLIGHTS)
        {
            u32 backlight_bits = readPowerManagement(PM_CONTROL_REG) & PM_BACKLIGHTS;
            if(backlight_bits == 12)
                REG_IPC_FIFO_TX = 2;
            else if(backlight_bits == 8 || backlight_bits == 4)
                REG_IPC_FIFO_TX = 1;
            else
                REG_IPC_FIFO_TX = 0;
            return;
        }    
        else if(msg == SET_BRIGHTNESS_0)
        {
            u32 reg_without_backlight = readPowerManagement(PM_CONTROL_REG) & ~PM_BACKLIGHTS;
            writePowerManagement(PM_CONTROL_REG, reg_without_backlight);
            return;
        }
        else if(msg == SET_BRIGHTNESS_1 ||
            msg == SET_BRIGHTNESS_2 ||
            msg == SET_BRIGHTNESS_3)
        {
            u32 reg_with_backlight = readPowerManagement(PM_CONTROL_REG) | PM_BACKLIGHTS;
            writePowerManagement(PM_CONTROL_REG, reg_with_backlight);
            return;
        }
        else if(msg == SET_BRIGHTNESS_NEXT ||
            msg == SET_BRIGHTNESS_PREVIOUS ||
            msg == SET_BACKLIGHTS_TOGGLE)
        {
            u32 oldreg = readPowerManagement(PM_CONTROL_REG);
            if(oldreg & PM_BACKLIGHTS) 
                oldreg &= ~PM_BACKLIGHTS;
            else
                oldreg |= PM_BACKLIGHTS;
            writePowerManagement(PM_CONTROL_REG, oldreg);
            return;
        }
        else if(msg == SET_BACKLIGHTS_ON)
        {
            u32 reg_with_backlight = readPowerManagement(PM_CONTROL_REG) | PM_BACKLIGHTS;
            writePowerManagement(PM_CONTROL_REG, reg_with_backlight);
            return;
        }
        else if(msg == SET_BACKLIGHTS_OFF)
        {
            u32 reg_without_backlight = readPowerManagement(PM_CONTROL_REG) & ~PM_BACKLIGHTS;
            writePowerManagement(PM_CONTROL_REG, reg_without_backlight);
            return;
        }
    } 
}
#endif // ARM7


#endif // _NDSX_BRIGHTNESS_

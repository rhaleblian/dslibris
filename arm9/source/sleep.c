
#if 0
void lidsleep(void) {
  __asm("mcr p15,0,r0,c7,c0,4");
  __asm("mov r0, r0");
  __asm("BX lr"); 
}

void lidsleep2(void) { 
  /* if lid 'key' */
  if (keysDown() & BIT(7)) {
    /* hinge is closed */
    /* set only key irq for waking up */
    unsigned long oldIE = REG_IE ;
    REG_IE = IRQ_KEYS ;
    *(volatile unsigned short *)0x04000132 = BIT(14) | 255 ; 
    /* any of the inner keys might irq */
    /* power off everything not needed */
    powerOFF(POWER_LCD) ;
    /* set system into sleep */
    lidsleep() ;
    /* wait a bit until returning power */
    while (REG_VCOUNT!=0) ;
    while (REG_VCOUNT==0) ;
    while (REG_VCOUNT!=0) ;
    /* power on again */
    powerON(POWER_LCD) ;
    /* set up old irqs again */
    REG_IE = oldIE ;
  }
}
#endif


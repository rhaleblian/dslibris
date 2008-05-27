// BGUI.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BGUI_H
#define BGUI_H

/* system includes */
/* (none) */

/* my includes */
#include "BScreen.h"
#include "BRunLoop.h"

/*!
  \brief Main GUI manager class (singleton)

  You cannot create BGUI instances yourself. There is only one
  instance, accessed using the get() method.
*/
class BGUI {
public:

  //! Return the global BGUI instance
  static BGUI* get();

  //! Add a screen (as an event responder)
  void addScreen(BScreen* screen);

  //! Enter the run loop
  void run();

  /*!
    \brief Repaint the GUI

    Normally, repainting is automatic. With this, you can issue 
    repaints when you want (and not when the GUI wants). If force is
    true, the GUI will be repainted even if it thinks it doesn't need
    to.
  */
  void repaint(bool force = false);

  //! Disable automatic repainting
  void disableRedraws();
  //! Enable automatic repainting
  void enableRedraws();
  //! Return whether automatic repainting is disabled
  bool redrawsDisabled() { return _redrawsDisabled; }

  //! Return the selected widget (if any)
  BWidget* selectedWidget() { return _selectedWidget; }
  //! Set the given widget as selected
  void selectWidget(BWidget* widget);

  /*!
    \brief Check whether the backlight is on

    This communicates with the arm7. The returned bool tells you
    whether the communication worked; the backlight flags are in the
    by-reference parameters top and bottom.
  */
  bool backlight(bool& top, bool& bottom);

  /*!
    \brief Switch the backlight off or on

    This communicates with the arm7. The returned bool tells you
    whether the communication worked.
  */
  bool switchBacklight(bool top, bool bottom);

  /*!
    \brief Query the backlight brightness

    This communicates with the arm7. The returned bool tells you
    whether the communication worked; the backlight brightness is in the
    by-reference parameter brightness.
  */
  bool backlightBrightness(u8& brightness);

  /*!
    \brief Set the backlight brightness
    
    This communicates with the arm7. The returned bool tells you
    whether the communication worked.
  */
  bool setBacklightBrightness(u8 brightness);

  /*!
    \brief Power off the DS
    
    This communicates with the arm7. The returned bool tells you
    whether the communication worked.
  */
  bool powerOff();

  //! Return the screens in the GUI
  const std::vector<BScreen*>& screens() { return _screens; }

protected:
  u32 sendFIFO(u32 command);

private:
  BGUI();
  static BGUI *_gui;

  BRunLoop* _runloop;

  BWidget *_selectedWidget;

  std::vector<BScreen*> _screens;

  bool _redrawsDisabled;
};

#endif /* BGUI_H */

// BDialog.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BDIALOG_H
#define BDIALOG_H

/* system includes */
/* (none) */

/* my includes */
#include "BWidget.h"
#include "BScreen.h"
#include "BRunLoop.h"

/*!
  \brief Class for on-screen dialogs

  You can cause a dialog to appear by calling its run() method and
  make it go away by calling stop().

  Don't use this class directly. Use some subclass of it.
*/
class BDialog:
  public BWidget
{
public:
  //! Construct a new dialog on the given screen
  BDialog(BScreen* screen, const BSize& size);
  virtual ~BDialog();

  //! Center the dialog on its screen
  void center();

  //! Run the dialog (starts a new run loop)
  virtual void run();

  //! Stop the dialog, which must be running
  virtual void stop();
  
protected:
  BRunLoop* _loop;
};

#endif /* BDIALOG_H */

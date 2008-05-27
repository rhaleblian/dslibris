// BMessagebox.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BMESSAGEBOX_H
#define BMESSAGEBOX_H

/* system includes */
/* (none) */

/* my includes */
#include "BButton.h"
#include "BDialog.h"

/*!
  \brief A simple message box

  This displays a text and can have an OK button, an additional Cancel
  button, or no buttons at all.
*/
class BMessagebox: public BDialog, public BButton::Delegate {
public:
  typedef enum {
    TYPE_NONE,      //! No buttons
    TYPE_OK,        //! An OK button
    TYPE_OK_CANCEL  //! An OK and a Cancel button
  } BMessageboxType;

  typedef enum {
    RESULT_OK,
    RESULT_CANCEL
  } BMessageboxResult;

  BMessagebox(BScreen* screen, const BSize& size,
	      const std::string& text,
	      BMessageboxType type);
  ~BMessagebox();

  /*!
    \brief Run the message box, blocking until a button is clicked

    If type is TYPE_NONE, this returns immediately. Use show() and
    BGUI::get()->repaint() instead.
  */
  void run(); 
  
  void onButtonClick(BButton* button);

  //! Return whether the user clicked OK or Cancel
  int result() { return _result; }

  //! Run a very simple dialog: Just a text and an OK button
  static void runMessage(BScreen* screen,
			 const std::string& txt,
			 BWidget::Rotation rot = BWidget::ROT_0);
private:
  int _result;
  BMessageboxType _type;
};

#endif /* BMESSAGEBOX_H */

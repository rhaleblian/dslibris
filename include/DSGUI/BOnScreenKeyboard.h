// OnScreenKeyboard.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef ONSCREENKEYBOARD_H
#define ONSCREENKEYBOARD_H

/* system includes */
/* (none) */

/* my includes */
#include "BButton.h"
#include "BLabel.h"
#include "BDialog.h"
#include "BKeyboard.h"

/*!
  \brief A dialog displaying a line of editable text and a keyboard
*/
class BOnScreenKeyboard:
  public BDialog,
  public BKeyboard::Delegate,
  public BButton::Delegate
{
public:
  BOnScreenKeyboard(BScreen* screen);
  ~BOnScreenKeyboard();

  void onKeyboardPressGlyph(BKeyboard* keyboard, const std::string& glyph);
  void onKeyboardPressSpecial(BKeyboard* keyboard,
			      BKeyboard::SpecialKey specialKey);
  void onButtonClick(BButton* button);

  //! Return the keyboard widget
  BKeyboard* keyboard() { return _keyboard; }

  //! Whether the user cancelled the dialog
  bool cancelled() { return _cancelled; }

  //! The edited text
  std::string text() { return _label->text(); }

  //! Set the text to edit
  void setText(const std::string& text) { _label->setText(text); }

  virtual void setRotation(Rotation rot);

private:
  BLabel* _label;
  BKeyboard* _keyboard;
  
  bool _cancelled;
};

#endif /* ONSCREENKEYBOARD_H */

// BKeyboard.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BKEYBOARD_H
#define BKEYBOARD_H

/* system includes */
#include <string>
#include <vector>
#include <deque>

/* my includes */
#include "BButton.h"


/*!
  \brief Keyboard class

  A layout contains multiple lines separated by newlines. The first
  line is a (max. 3-char) label. The following lines contain one char
  for each keyboard key. The meaning for the chars is as follows:

    - Numbers represent themselves ("123" gives you a "1", "2", and
      "3" button in a row)
    - Symbols represent themselves
    - Lowercase ASCII represent themselves and will be shifted
    - Non-ASCII represent themselves, but cannot be shifted
    - A space (ASCII 32) represents itself
    - Uppercase ASCII represent special functions and are coded as
      follows (labels depending on available space in brackets):

      -# S: Shift ["Shift", "Shft", "^"]
      -# L: Caps Lock ["Caps Lock", "Lock", "L"]
      -# K: Backspace ("Kill") ["Backspace", "Bksp", "<-"]
      -# E: Del ("Erase") ["Delete", "Del", "X"]
      -# R: Return ["Return", "Ret", "R"]
      -# N: Switch to next keyboard layout [that layout's label]
      -# A-D: Switch to keyboard layout 0-3, accordingly [layout's label]

  More than one same character on a row or a column will be grouped
  together as one single button (i.e. "KK" gives you a 2-cell-wide
  Backspace button). If this makes a widget higher than wide and the
  widget's label doesn't fit left-to-right, text direction will be
  bottom-to-top. Buttons cannot go around corners! I.e. don't
  construct something as this:

  \code
                    abcR1
                    deRR2
  \endcode

  This will not give you a right-angled return button, but rather
  something undefined.
*/
  
class BKeyboard:
  public BWidget,
  public BButton::Delegate
{
public:
  typedef enum {
    KEY_SHIFT = 'S',
    KEY_CAPSLOCK = 'L',
    KEY_BACKSPACE = 'K',
    KEY_DEL = 'E',
    KEY_RETURN = 'R',
    KEY_NEXTLAYOUT = 'N',
    KEY_CHOOSELAYOUT_0 = 'A',
    KEY_CHOOSELAYOUT_1 = 'B',
    KEY_CHOOSELAYOUT_2 = 'C',
    KEY_CHOOSELAYOUT_3 = 'D'
  } SpecialKey;
  
  class Delegate {
  public:
    virtual ~Delegate() {}
    virtual bool validateButton(BButton* button) { return true; }
    virtual void onKeyboardPressGlyph(BKeyboard* keyboard,
				      const std::string& glyph) {}
    virtual void onKeyboardPressSpecial(BKeyboard* keyboard,
					SpecialKey specialKey) {}
  };

  static std::string qwertzLayout;
  static std::string numLayout;
  static std::string specialLayout;

  static std::vector<std::string> defaultLayouts();

  BKeyboard(BWidget* parent, const BRect& frame);

  void setLayout(const std::string& layout);
  void setLayouts(const std::vector<std::string>& layouts);
  void chooseLayoutNum(unsigned int num);
  void chooseNextLayout();

  void onButtonClick(BButton* button);

  void setDelegate(Delegate* deleg);
  Delegate* delegate() { return _deleg; }

  static std::deque<std::string> loadLayouts(BVirtualFile* file);

protected:
  BSize sizeOfLayoutCell(const std::string& layout);
  void setupSpecialButton(BButton* button);
  void uppercaseVisibleButtons();
  void lowercaseVisibleButtons();
  
  std::vector<std::vector<BButton*> > _buttons;
  std::vector<std::string> _layouts;
  Delegate* _deleg;
  unsigned int _curLayoutnum;
};

#endif /* BKEYBOARD_H */

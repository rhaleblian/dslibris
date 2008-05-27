/* This is -*- C -*-)

   BButton.h   
   \author: Bjoern Giesler <bjoern@giesler.de>
   
   
   
   $Author: giesler $
   $Locker$
   $Revision$
   $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $
 */

#ifndef BBUTTON_H
#define BBUTTON_H

/* system includes */
#include <string>

/* my includes */
#include "BLabel.h"
#include "BGraphics.h"
#include "BFont.h"

/*!
  \brief Push- or Radio-Button class
*/

class BButton: public BLabel {
 public:
  class Delegate {
  public:
    virtual ~Delegate() {}
    virtual void onButtonClick(BButton* button) {}
  };

  typedef enum {
    BT_TRIGGER, //! Push button
    BT_TOGGLE   //! Toggle button
  } ButtonType;
  
  BButton(BWidget* parent, const BRect& frame,
	  ButtonType type = BT_TRIGGER);
  void setDelegate(Delegate* deleg);
  Delegate* delegate() { return _deleg; }

  virtual void setType(ButtonType type);
  virtual ButtonType type() const { return _type; }

  virtual bool value() const { return _value; }
  virtual void setValue(bool value);
  
  virtual void draw(BImage& img);

  virtual void handleEvent(const BEvent& event);
  
 private:
  Delegate* _deleg;
  bool _touchdown;
  ButtonType _type;
  bool _value;
};

#endif /* BBUTTON_H */

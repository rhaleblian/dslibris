/* This is -*- C -*-)

   BRadioGroup.h   
   \author: Bjoern Giesler <bjoern@giesler.de>
   
   
   
   $Author: giesler $
   $Locker$
   $Revision$
   $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $
 */

#ifndef BRADIOGROUP_H
#define BRADIOGROUP_H

/* system includes */
/* (none) */

/* my includes */
#include "BWidget.h"
#include "BButton.h"

class BRadioGroup: public BButton::Delegate, public BWidget {
 public:
  class Delegate {
  public:
    virtual ~Delegate() {}
    virtual void onSelectButton(BButton* button) {}
  };
  
  BRadioGroup(BWidget* parent, const BRect& frame);

  virtual void setDelegate(Delegate* deleg);
  virtual Delegate* delegate() { return _deleg; }

  void selectButton(BButton* button);
  void selectButtonWithTag(int tag);
  BButton* selectedButton();

  virtual void registerChildren();

  virtual void onButtonClick(BButton* button);
  
 protected:
  Delegate* _deleg;
};

#endif /* BRADIOGROUP_H */

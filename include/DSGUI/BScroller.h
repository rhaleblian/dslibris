/* This is -*- C -*-)

   BScroller.h   
   \author: Bjoern Giesler <bjoern@giesler.de>
   
   
   
   $Author: giesler $
   $Locker$
   $Revision$
   $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $
 */

#ifndef BSCROLLER_H
#define BSCROLLER_H

/* system includes */
/* (none) */

/* my includes */
#include "BWidget.h"

class BScrollable;

class BScroller: public BWidget {
 public:
  typedef enum {
    ORIENT_VERTICAL,
    ORIENT_HORIZONTAL
  } Orientation;

  int lastpos;
  BPoint lastpoint;
  
  BScroller(BWidget* parent, const BRect& frame);

  void setOrientation(Orientation orient);
  Orientation orientation(void) const { return _orient; }

  void setStepIncrement(unsigned int stepIncr);
  unsigned int stepIncrement(void) const { return _stepIncr; }

  void setPageIncrement(unsigned int pageIncr);
  unsigned int pageIncrement(void) const { return _pageIncr; }

  void setPosition(unsigned int position);
  unsigned int position(void) const { return _position; }

  void setMinimum(unsigned int minimum);
  unsigned int minimum(void) const { return _minimum; }

  void setMaximum(unsigned int maximum);
  unsigned int maximum(void) const { return _maximum; }

  void setAmountRepresentedByThumb(unsigned int amountRepr);
  unsigned int amountRepresentedByThumb(void) const { return _amountRepr; }

  virtual void draw(BImage& img);

  virtual void handleEvent(const BEvent& event);

  void setScrollable(BScrollable *scrollable);

 protected:
  unsigned int _stepIncr, _pageIncr;
  unsigned int _position, _minimum, _maximum, _amountRepr;
  Orientation _orient;
  bool _dragged; BPoint _dragOrigin;
  BScrollable *_scrollable;
};

#endif /* BSCROLLER_H */

/* This is -*- C++ -*-

   BScrollable.h   
   \author: Bjoern Giesler <bjoern@giesler.de>
   
   
   
   $Author: giesler $
   $Locker$
   $Revision$
   $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $
 */

#ifndef BSCROLLABLE_H
#define BSCROLLABLE_H

/* system includes */
/* (none) */

/* my includes */
#include "BWidget.h"

class BScroller;

class BScrollable: public BWidget {
public:
  BScrollable(BWidget* parent, const BRect& frame);

  void setHScroller(BScroller *scroller);
  void setVScroller(BScroller *scroller);

  virtual void updateScroller(BScroller *scroller) = 0;
  virtual void scrollToPosition(BScroller *scroller,
				unsigned int position) = 0;
protected:
  BScroller *_hScroller, *_vScroller;
};

#endif /* BSCROLLABLE_H */

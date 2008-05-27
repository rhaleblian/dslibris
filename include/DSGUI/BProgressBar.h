// BProgressBar.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BPROGRESSBAR_H
#define BPROGRESSBAR_H

/* system includes */
/* (none) */

/* my includes */
#include "BWidget.h"

class BProgressBar: public BWidget {
public:
  BProgressBar(BWidget* parent, const BRect& frame);

  /*!
    \brief Set total value.

    This destroys all tick information! Re-set ticks after calling
    this.
  */
  void setTotal(int total);
  int total() { return _total; }

  void setProgress(int progress);
  int progress() { return _progress; }
  
  virtual void draw(BImage& img);

protected:
  int _progress, _total;
};

#endif /* BPROGRESSBAR_H */

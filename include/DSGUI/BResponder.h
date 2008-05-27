// BResponder.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BRESPONDER_H
#define BRESPONDER_H

/* system includes */
/* (none) */

/* my includes */
#include "BEvent.h"

/*!
  \brief Base class for event responders

  A responder can select whether it accepts input or not; by default,
  it does accept input.
*/
class BResponder {
public:
  BResponder() { _accepts = true; }
  virtual ~BResponder() {}

  //! Handle the given event. Override if necessary
  virtual void handleEvent(const BEvent& event) {};

  virtual void setAcceptsInput(bool yn) { _accepts = yn; }
  virtual bool acceptsInput() { return _accepts; }
protected:
  bool _accepts;
};

#endif /* BRESPONDER_H */

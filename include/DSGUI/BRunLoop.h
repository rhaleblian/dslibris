// BRunLoop.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BRUNLOOP_H
#define BRUNLOOP_H

/* system includes */
#include <vector>

/* my includes */
#include "BEvent.h"
#include "BResponder.h"

class BRunLoop {
  friend class BGUI;
public:
  class VBlankReceiver {
  public:
    virtual ~VBlankReceiver() {}
    virtual void runloopReachedVBlank(BRunLoop *loop) {}
  };

  BRunLoop() { }
  
  void addResponder(BResponder *responder);
  void removeResponder(BResponder *responder);

  void addVBlankReceiver(VBlankReceiver *receiver);
  void removeVBlankReceiver(VBlankReceiver *receiver);

  static void addGlobalVBlankReceiver(VBlankReceiver *receiver);
  static void removeGlobalVBlankReceiver(VBlankReceiver *receiver);

  void run();
  void stop();

  void postEvent(const BEvent& event);

  static BRunLoop* currentRunLoop();

  uint32 buttonsHeld() { return _buttons; }

protected:
  //! Check keys & touchscreen and post events
  void handle();
  
  static std::vector<VBlankReceiver*> _globalreceivers;
  static BRunLoop* _current;

  std::vector<VBlankReceiver*> _receivers;
  std::vector<BResponder*> _responders;
  bool _shouldStop;
  static uint32 _buttons;
  static bool _touchdown;
  static int _touchx, _touchy;
};

#endif /* BRUNLOOP_H */

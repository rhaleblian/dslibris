// BAnimation.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BANIMATION_H
#define BANIMATION_H

/* system includes */
/* (none) */

/* my includes */
#include "BRunLoop.h"

class BAnimation: public BRunLoop::VBlankReceiver {
public:
  
  /*!
   \name Fixed-point arithmetic
   \{
  */
  typedef s32 FIXED;
  static u8 FIX_SHIFT;
  static FIXED FIX_SCALE;
  static float FIX_SCALEF; 
  static float FIX_SCALEF_; 
  static inline FIXED INT2FIX(int _i) { return ((_i)<<FIX_SHIFT); }
  static inline int FIX2INT(FIXED _n) { return ((_n)>>FIX_SHIFT); }
  static inline FIXED FIX_FRAC(FIXED _n) { return ((_n)&(FIX_SCALE-1)); }

  static inline FIXED FLOAT2FIX(float _f) { return (FIXED((_f)*FIX_SCALEF)); }
  static inline float FIX2FLOAT(FIXED _n) { return ((_n)*FIX_SCALEF_); }

  static inline FIXED FIX_ADD(FIXED a, FIXED b) { return a+b; }
  static inline FIXED FIX_SUB(FIXED a, FIXED b) { return a-b; }
  static inline FIXED FIX_MUL(FIXED a, FIXED b) { return (a*b)>>FIX_SHIFT; }
  static inline FIXED FIX_DIV(FIXED a, FIXED b) { return (a<<FIX_SHIFT)/b; }
  /*!
    \}
  */

class Delegate {
  public:
    virtual ~Delegate() {}
    virtual void animationDidStart(BAnimation* animation) {}
    virtual void animationDidChangeValue(BAnimation* animation, FIXED value) {}
    virtual void animationDidFinish(BAnimation* animation) {}
  };
  
  BAnimation();
  ~BAnimation();

  typedef FIXED (* FixedModifier)(FIXED);

  void addTarget(volatile uint32* target, FixedModifier modifier = NULL);
  void addTarget(volatile uint16* target, FixedModifier modifier = NULL);
  void clearTargets();
		 
  static FIXED pageflipModifierYDY(FIXED value);
  static FIXED pageflipModifierCY(FIXED value);

  void setStartValue(int value);
  void setStartValue(float value);
  FIXED startValue() { return _start; }

  void setStepValue(int value);
  void setStepValue(float value);
  FIXED stepValue() { return _step; }

  void setEndValue(int value);
  void setEndValue(float value);
  FIXED endValue() { return _end; }

  FIXED value() { return _value; }

  //! Schedule the animation in the current run loop.
  void schedule(bool global = false);
  //! Unschedule the animation. Happens automatically if end is reached.
  void unschedule();

  //! Schedule the animation in its own run loop. Returns after unschedule().
  void run();

  void runloopReachedVBlank(BRunLoop* loop);

  void setDelegate(Delegate* deleg);
  Delegate* delegate() { return _deleg; }

private:
  void setTarget(FIXED value);

  typedef struct {
    volatile void* target;
    bool targetIs16Bit;
    FixedModifier modifier;
  } AnimTarget;

  std::vector<AnimTarget> _targets;
  FIXED _start, _end, _step, _value;
  bool _running, _global;
  Delegate* _deleg;
  BRunLoop* _loop;
};

#endif /* BANIMATION_H */

/* This is -*- C -*-)

   BScreen.h   
   \author: Bjoern Giesler <bjoern@giesler.de>
   
   
   
   $Author: giesler $
   $Locker$
   $Revision$
   $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $
 */

#ifndef BSCREEN_H
#define BSCREEN_H

/* system includes */
#include <vector>

/* my includes */
#include "BWidget.h"

class BScreen: public BResponder {
 public:
  BScreen(BImage* image);
  BScreen(uint16* address, BSize size);
  ~BScreen();

  uint16 backgroundColor() { return _bg; }
  void setBackgroundColor(uint16 color);
  
  void setReactsToTouch(bool reactsToTouch);
  bool reactsToTouch() { return _touch; }

  void addWidget(BWidget* widget);
  void removeWidget(BWidget* widget);
  const std::vector<BWidget*>& widgets() { return _widgets; }

  void setNeedsRedrawOnAllWidgetsCoveredBy(const BRect& rect, bool needs);

  void setDialog(BWidget* dialog);
  BWidget* dialog() { return _dialog; }

  void clear();
  void repaint(bool force=false);

  BImage* image() { return _img; }

  void handleEvent(const BEvent& event);
  
 private:
  BImage* _img; bool _freeImg;
  std::vector<BWidget*> _widgets;
  BWidget* _dialog;
  bool _forceNextRedraw;
  bool _touch;
  uint16 _bg;
};

#endif /* BSCREEN_H */

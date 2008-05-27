// BTheme.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BTHEME_H
#define BTHEME_H

/* system includes */
/* (none) */

/* my includes */
#include "BImage.h"
#include "BGraphics.h"
#include "BWidget.h"
#include "BScroller.h"

class BFont;

class BTheme {
public:
  virtual ~BTheme() {}
  virtual BRect contentRectForWidget(const BWidget& widget) = 0;
  virtual void drawWidgetBackground(BImage& img, BWidget& widget) = 0;
  virtual void drawWidgetFrame(BImage& img, BWidget& widget) = 0;

  virtual void drawButton(BImage& img, BWidget& w) = 0;

  virtual void drawSelectionRect(BImage& img, const BRect& rect,
				 const BWidget& w) = 0;

  virtual void drawScroller(BImage& img, BScroller& w) = 0;
  virtual int scrollerHitTest(BScroller& w, const BEvent& event) = 0;

  virtual BFont *defaultFont() = 0;
  virtual BFont *defaultSmallFont() = 0;
  virtual BFont *defaultBigFont() = 0;

  virtual uint16 textColor() = 0;
  virtual uint16 selectedTextColor() = 0;

  static BTheme* currentTheme();
private:
  static BTheme *current;
};

class BSimpleTheme: public BTheme {
public:
  BSimpleTheme();
  virtual ~BSimpleTheme();
  virtual BRect contentRectForWidget(const BWidget& widget);
  virtual void drawWidgetBackground(BImage& img, BWidget& widget);
  virtual void drawWidgetFrame(BImage& img, BWidget& widget);

  virtual void drawButton(BImage& img, BWidget& w);

  virtual void drawSelectionRect(BImage& img, const BRect& rect,
				 const BWidget& w);

  virtual void drawScroller(BImage& img, BScroller& w);
  virtual int scrollerHitTest(BScroller& w, const BEvent& event);

  virtual BFont *defaultFont() { return _font; }
  virtual BFont *defaultSmallFont() { return _smallFont; }
  virtual BFont *defaultBigFont() { return _bigFont; }

  virtual uint16 textColor() { return RGB15(0, 0, 0) | BIT(15); }
  virtual uint16 selectedTextColor() { return RGB15(31, 31, 31) | BIT(15); }

private:
  BFont *_font, *_smallFont, *_bigFont;
};


#endif /* BTHEME_H */

// BText.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BTEXT_H
#define BTEXT_H

/* system includes */
/* (none) */

/* my includes */
#include "BWidget.h"
#include "BScrollable.h"

class BText: public BScrollable {
public:
  BText(BWidget* parent, const BRect& frame);
  ~BText();

  void setFirstLineColor(uint16 color) {
    _firstlinecolor = color;
    _firstlinecolorset = true;
    setNeedsRedraw(true);
  }
  void resetFirstLineColor() {
    _firstlinecolorset = false;
    setNeedsRedraw(true);
  }
  
  //! BText doesn't support setTextDirection().
  void setTextDirection(BWidget::TextDirection dir) {}

  void setText(const std::string& text);
  bool full() { return _full; }
  int indexOfLastCharacterOnScreen();

  void setStripSingleNewlines(bool stripSingleNL);
  bool stripSingleNewlines() { return _stripSingleNL; }

  virtual void setFont(BFont* font);

  virtual void draw(BImage& img);

  void clear();

  void setSlaveText(BText* slave);
  BText *slaveText() { return _slave; }

  virtual void updateScroller(BScroller *scroller);
  virtual void scrollToPosition(BScroller *scroller,
				unsigned int position);
  
  class Line {
  public:
    virtual ~Line() {}
    unsigned int index, length;
    int spacingBefore;
    int leftIndent, rightIndent;
    int height;
    uint16 color;
    BWidget::TextAlignment align;

    virtual void draw(BText* text, BImage& img, const BRect& clip, BPoint& pt,
		      uint16 color = RGB15(0, 0, 0)|BIT(15)) = 0;
  };

  class StringLine: public Line {
  public:
    std::string line;
    virtual void draw(BText* text, BImage& img, const BRect& clip, BPoint& pt,
		      uint16 color = RGB15(0, 0, 0)|BIT(15));
  };

protected:
  void rewrap(unsigned int index, unsigned int length);

  std::string _text;
  bool _full;

  uint16 _firstlinecolor;
  bool _firstlinecolorset;

  bool _stripSingleNL;

  std::vector<Line*> _lines;
  int _firstline;
  BText* _slave;
};

#endif /* BTEXT_H */

// BLabel.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BLABELWIDGET_H
#define BLABELWIDGET_H

/* system includes */
#include <nds.h>
#include <string>

/* my includes */
#include "BWidget.h"
#include "BFont.h"

/*!
  \brief Single-line text field
*/
class BLabel: public BWidget {
public:
  BLabel(BWidget* parent, const BRect& frame);

  virtual void draw(BImage& img);

  const std::string& text() const { return _text; }
  void setText(const std::string& text);

  bool drawsCursor() { return _drawsCursor; }
  void setDrawsCursor(bool dc) { _drawsCursor = dc; }

  void setEditable(bool editable);
  bool editable() { return _editable; }
  
  void handleEvent(const BEvent& event);

protected:
  virtual void drawText(BImage& img);

private:
  std::string _text;
  bool _drawsCursor;
  bool _editable;
};

#endif /* BLABELWIDGET_H */

// BWidget.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BWIDGET_H
#define BWIDGET_H

/* system includes */
#include <nds.h>
#include <vector>

/* my includes */
#include "BGraphics.h"
#include "BImage.h"
#include "BResponder.h"

class BFont;
class BScreen;

/*!
  \brief Widget base class

  All widgets inherit from this.
*/
class BWidget: public BResponder {
public:
  //! Widget state
  typedef enum {
    STATE_DISABLED, //! non-selectable, greyed out, ...
    STATE_ENABLED,  //! regular, sitting there waiting
    STATE_SELECTED, //! selected as keyboard focus
    STATE_ACTIVE    //! performing some action
  } State;

  //! Rotation of the widget coordinate system relative to parent
  typedef enum {
    ROT_0 = 0,
    ROT_90 = 1,
    ROT_180 = 2,
    ROT_270 = 3
  } Rotation;

  //! Direction for text drawing
  typedef enum {
    DIR_LEFTTORIGHT = 0,
    DIR_TOPTOBOTTOM = 1,
    DIR_RIGHTTOLEFT = 2,
    DIR_BOTTOMTOTOP = 3
  } TextDirection;

  //! Horizontal alignment for text drawing
  typedef enum {
    ALIGN_CENTER,
    ALIGN_LEFT,
    ALIGN_RIGHT
  } TextAlignment;

  //! Relief (border) type
  typedef enum {
    RELIEF_NONE,   //! No border at all
    RELIEF_SIMPLE, //! Plain rectangle
    RELIEF_RAISED, //! "Raised 3D" border
    RELIEF_SUNKEN  //! "Sunken 3D" border
  } ReliefType;

  /*!
    \brief Construct a widget

    If this is a toplevel widget, specify NULL as the parent. The
    frame is given in parent coordinates.
  */
  BWidget(BWidget* parent, const BRect& frame);
  virtual ~BWidget();

  //! Return the frame in parent coordinates
  const BRect& frame() const { return _frame; }
  //! Set the frame in parent coordinates
  virtual void setFrame(const BRect& frame);

  //! Return the widget's current state
  virtual State state() const { return _state; }
  //! Set the widget's current state
  virtual void setState(State state) { _state = state; _needsRedraw = true; }

  void setTag(int tag) { _tag = tag; }
  int tag() { return _tag; }
  
  virtual TextDirection textDirectionRelativeToScreen() const;
  virtual TextDirection textDirection() const { return _tdir; }
  virtual void setTextDirection(TextDirection dir) { _tdir = dir; _needsRedraw = true; }

  /*!
    \name Coordinate transforms in the widget hierarchy
    \{
  */
  
  virtual Rotation rotation() const { return _rot; }
  virtual void setRotation(Rotation rot);
  virtual Rotation accumulatedRotation() const;

  /*!
    \brief Transform the given point up in the widget hierarchy

    This will transform the point, given in widget coordinates, into
    parent (global==false) or root (global==true) coordinates.
  */
  inline BPoint transformUp(BPoint pt, bool global = true)
    const
  {
    BPoint pt1;
    switch(_rot) {
    case ROT_0:
      pt1.x = pt.x + _frame.pt.x;
      pt1.y = pt.y + _frame.pt.y;
      break;

    case ROT_90:
      pt1.x = _frame.pt.x - pt.y + _frame.sz.width;
      pt1.y = _frame.pt.y + pt.x;
      break;

    case ROT_180:
      pt1.x = _frame.pt.x - pt.x + _frame.sz.width;
      pt1.y = _frame.pt.y - pt.y + _frame.sz.height;
      break;
      
    case ROT_270:
      pt1.x = _frame.pt.x + pt.y;
      pt1.y = _frame.pt.y - pt.x + _frame.sz.height;
      break;
    }

    if(_parent && global)
      return _parent->transformUp(pt1);
    return pt1;
  }

  /*!
    \brief Transform the given point down in the widget hierarchy

    This will transform the point, given in the parent's
    (global==false) or root (global==true) coordinates, into
    widget coordinates.
  */
  inline BPoint transformDown(const BPoint& pt, bool global = true)
    const
  {
    BPoint pt1 = pt, pt2;

    if(_parent && global)
      pt1 = _parent->transformDown(pt1);

    switch(_rot) {
    case ROT_0:
      pt2.x = pt1.x - _frame.pt.x;
      pt2.y = pt1.y - _frame.pt.y;
      break;

    case ROT_90:
      pt2.x = pt1.y - _frame.pt.y;
      pt2.y = _frame.pt.x + _frame.sz.width - pt1.x;
      break;

    case ROT_180:
      pt2.x = _frame.pt.x + _frame.sz.width - pt1.x;
      pt2.y = _frame.pt.y + _frame.sz.height - pt1.y;
      break;
      
    case ROT_270:
      pt2.x = _frame.pt.y + _frame.sz.height - pt1.y;
      pt2.y = pt1.x - _frame.pt.x;
      break;
    }

    return pt2;
  }

  /*!
    \brief Transform the given rect up in the widget hierarchy

    This will transform the rect, given in widget coordinates, into
    parent (global==false) or root (global==true) coordinates.
  */
  inline BRect transformUp(const BRect& r, bool global = true)
    const
  {
    return BRect(transformUp(r.topLeft(), global),
		 transformUp(r.bottomRight(), global));
  }

  /*!
    \brief Transform the given rect down in the widget hierarchy

    This will transform the rect, given in the parent's
    (global==false) or root (global==true) coordinates, into
    widget coordinates.
  */
  inline BRect transformDown(const BRect& r, bool global = true)
    const
  {
    return BRect(transformDown(r.topLeft(), global),
		 transformDown(r.bottomRight(), global));
  }
    
  /*!
    \}
  */
  
  TextAlignment textAlignment() const { return _align; }
  virtual void setTextAlignment(TextAlignment align);

  ReliefType reliefType() const { return _relief; }
  virtual void setReliefType(ReliefType relief);

  unsigned int vPadding() const { return _vpadding; }
  virtual void setVPadding(unsigned int padding);

  unsigned int hPadding() const { return _hpadding; }
  virtual void setHPadding(unsigned int padding);

  /*!
    \name Debug drawing

    This flag causes a 5-pixel grid and a widget coordinate system
    (red axis = x, green axis = y) to be drawn in the widget, for
    easier draw debugging and children positioning

    \{
  */
    
  bool drawsDebugStuff() const { return _debug; }
  virtual void setDrawsDebugStuff(bool debug);

  /*!
    \}
  */

  //! Draw the widget onto the given image
  virtual void draw(BImage& img);

  //! Set or reset the redraw flag
  virtual void setNeedsRedraw(bool needs);

  //! Returns true if the widget or one of its children needs redraw
  virtual bool needsRedraw();

  virtual bool isShown() {
    if(_parent) return _parent->isShown() && _shown;
    return _shown;
  }
  virtual void hide() { _shown = false; }
  virtual void show();

  /*!
    \name Drawing attributes
    \{
  */
  uint16 backgroundColor() const { return _bgcolor; }
  virtual void setBackgroundColor(uint16 color, bool recursive = false);

  uint8 backgroundAlpha() const { return _bgalpha; }
  virtual void setBackgroundAlpha(uint8 alpha);
  
  uint8 widgetAlpha() const { return _widgetalpha; }
  virtual void setWidgetAlpha(uint8 alpha);

  BFont *font() const { return _font; }
  virtual void setFont(BFont* font);

  uint16 textColor() { return _textcolor; }
  virtual void setTextColor(uint16 color, bool recursive = false);

  uint16 selectedTextColor() { return _selectedtextcolor; }
  virtual void setSelectedTextColor(uint16 color);

  /*!
    \}
  */

  void handleEvent(const BEvent& event);

  /*!
    \name Children, parents, and screens

    \{
  */

  //! Return the widget's children vector
  std::vector<BWidget*>& children() { return _children; }

  //! Whether the widget deletes its children
  bool deletesChildren() { return _deleteChildren; }
  //! Set the widget to delete its children upon destruction
  void setDeletesChildren(bool yesno) { _deleteChildren = yesno; }
  //! Tell the widget to delete its children now
  void deleteChildren();

  //! Return the widget's parent, or NULL
  BWidget* parent() { return _parent; }

  //! Return the widget's screen (or it's parent's screen, recursively), or NULL
  BScreen* screen();
  //! Set the widget's screen. Don't use directly.
  void setScreen(BScreen* screen);

  /*!
    \}
  */
protected:
  std::vector<BWidget*> _children;
  virtual void addChild(BWidget* w);
  virtual void removeChild(BWidget* w);
  
  BRect _frame;
  ReliefType _relief;
  unsigned int _vpadding, _hpadding;
  uint16 _bgcolor;
  uint16 _textcolor, _selectedtextcolor;
  uint8 _bgalpha;
  uint8 _widgetalpha;
  bool _needsRedraw;
  bool _debug;
  bool _shown;

  BFont *_font;
  BWidget *_parent;
  BScreen *_screen;

  State _state;
  TextDirection _tdir;
  Rotation _rot;
  TextAlignment _align;
  int _tag;
  bool _deleteChildren;
};

#endif /* BWIDGET_H */

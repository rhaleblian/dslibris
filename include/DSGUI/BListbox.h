/* This is -*- C -*-)

   BListbox.h   
   \author: Bjoern Giesler <bjoern@giesler.de>
   
   
   
   $Author: giesler $
   $Locker$
   $Revision$
   $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $
 */

#ifndef BLISTBOX_H
#define BLISTBOX_H

/* system includes */
#include <string>
#include <deque>

/* my includes */
#include "BScrollable.h"
#include "BFont.h"

class BListbox: public BScrollable {
 public:
  typedef struct {
    std::string title;
    u16 color;
    int tag;
  } Entry;

  class Delegate {
  public:
    virtual ~Delegate() {};
    virtual void onEntryClick(BListbox *box, Entry &entry) {};
    virtual void onEntryDoubleClick(BListbox *box, Entry &entry) {};
  };

  BListbox(BWidget* parent, const BRect &frame);

  void setDelegate(Delegate* deleg);
  Delegate* delegate() { return _deleg; }
  
  void addEntry(const Entry &entry);
  /*!
    \brief Add entry with given title, color, and tag.

    Use the special color value 0 to make the given entry use the
    widget's textcolor value.
  */
  void addEntry(const std::string &title,
		u16 color = 0,
		int tag = 0)
  {
    Entry entry; entry.title = title; entry.color = color; entry.tag = tag;
    addEntry(entry);
  }

  void clear();

  virtual void setEntrySpacing(int spacing);
  virtual int entrySpacing();

  // from BWidget
  virtual void draw(BImage &img);
  
  // from BScrollable
  virtual void updateScroller(BScroller *scroller);
  virtual void scrollToPosition(BScroller *scroller,
				unsigned int position);

  void handleEvent(const BEvent& event);

  const Entry& selectedEntry();

  void selectEntry(const std::string& name);
  void selectEntry(int index);
  void selectEntryWithTag(int tag);

  bool drawsSelection() { return _drawsSelection; }
  void setDrawsSelection(bool draws) {
    _drawsSelection = draws;
    setNeedsRedraw(true);
  }

 private:
  std::deque<Entry> _entries;
  unsigned int _firstIndex;
  unsigned int _selectedIndex;
  int _spacing;
  bool _drawsSelection;
  Delegate* _deleg;
};

#endif /* BLISTBOX_H */

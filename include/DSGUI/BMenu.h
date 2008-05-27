// BMenu.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BMENU_H
#define BMENU_H

/* system includes */
/* (none) */

/* my includes */
#include "BListbox.h"
#include "BDialog.h"

/*!
  \brief A dialog displaying a list of options
*/
class BMenu:
  public BDialog,
  public BListbox::Delegate
{
public:
  BMenu(BScreen* screen, const BSize& size);
  ~BMenu();

  void addEntry(const std::string &title,
		u16 color = RGB15(0, 0, 0)|BIT(15),
		int tag = 0);
  const BListbox::Entry& selectedEntry();
  void setEntrySpacing(int spacing) { _lb->setEntrySpacing(spacing); }
  int entrySpacing() { return _lb->entrySpacing(); }
  
  void onEntryDoubleClick(BListbox *box, BListbox::Entry &entry);
  void onEntryClick(BListbox *box, BListbox::Entry &entry);

  void setRotation(BWidget::Rotation rot);

  void handleEvent(const BEvent& event);

  bool cancelled() { return _cancelled; }

  virtual void run();
private:
  BListbox* _lb;
  bool _cancelled;
};

#endif /* BMENU_H */

/* This is -*- C++ -*-)

   BTabbedWidget.h   
   \author: Bjoern Giesler <bjoern@giesler.de>
   
   
   
   $Author: giesler $
   $Locker$
   $Revision$
   $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $
 */

#ifndef BTABBEDWIDGET_H
#define BTABBEDWIDGET_H

/* system includes */
/* (none) */

/* my includes */
#include "BWidget.h"
#include "BRadioGroup.h"

class BTabbedWidget:
  public BWidget,
  public BRadioGroup::Delegate
{
 public:
  class Delegate {
  public:
    virtual ~Delegate() {}
    virtual void onSelectTabbedPane(BTabbedWidget* tabbedwidget,
				    const std::string& title,
				    BWidget* pane) {}
  };
  
  BTabbedWidget(BWidget* parent, const BRect& frame);

  BWidget* addTabbedPane(const std::string& title);
  void onSelectButton(BButton* button);

  void setDelegate(Delegate* deleg);
  Delegate* delegate() { return _deleg; }

 protected:
  typedef struct {
    BButton* tab;
    BWidget* pane;
  } TabAndPane;

  std::vector<TabAndPane> _tabs;
  BRadioGroup *_rgrp;
  Delegate* _deleg;
};

#endif /* BTABBEDWIDGET_H */

// BFileSelector.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BFILESELECTOR_H
#define BFILESELECTOR_H

/* system includes */
/* (none) */

/* my includes */
#include "BDialog.h"
#include "BButton.h"
#include "BListbox.h"
#include "BScroller.h"

/*!
  \brief A file selector dialog

  By default, this will let you select any file. Once you start adding
  file types with the addFileType() method, it will only let you
  select files with those suffixes.
*/
class BFileSelector:
  public BDialog,
  public BButton::Delegate,
  public BListbox::Delegate
{
public:
  BFileSelector(BScreen* screen, const std::string& startingDirectory = "");
  ~BFileSelector();

  //! Return whether the user cancelled the dialog
  bool cancelled() { return _cancelled; }

  //! Return the name of the selected file (as a normalized full path)
  std::string selectedFilename();

  void onButtonClick(BButton* button);
  void onEntryClick(BListbox* box, BListbox::Entry& entry);
  void onEntryDoubleClick(BListbox* box, BListbox::Entry& entry);

  //! Add a file type the selector will accept
  void addFileType(const std::string& filetype);

  //! Run the dialog
  virtual void run();

protected:
  void fillBox();
  
private:
  BScroller* _scroller;
  BListbox* _box;
  BButton *_ok, *_cancel, *_dirhier;
  BLabel *_toplabel;
  bool _cancelled;
  std::string _curdir;
  std::string _selected;
  std::vector<std::string> _filetypes;
};

#endif /* BFILESELECTOR_H */

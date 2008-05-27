// BFontSelector.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BFONTSELECTOR_H
#define BFONTSELECTOR_H

/* system includes */
/* (none) */

/* my includes */
#include "BDialog.h"
#include "BButton.h"
#include "BListbox.h"
#include "BScroller.h"
#include "BLabel.h"
#include "BText.h"

/*!
  \brief Font selection dialog
*/
class BFontSelector:
  public BDialog,
  public BListbox::Delegate,
  public BButton::Delegate
{
public:
  BFontSelector(BScreen* screen);

  //! Whether the user cancelled the dialog
  bool cancelled() { return _cancelled; }

  //! The selected font
  BFont* font() { return _font; }

  //! Set the current font for the dialog to display
  void setFont(BFont* font);
  
  void onEntryClick(BListbox* box, BListbox::Entry& e);
  void onButtonClick(BButton* button);

  void setRotation(BWidget::Rotation rot);
private:
  void updateSizesBox();
  void updateVariants();
  void updatePreview();
  
  BListbox *_namesBox, *_sizesBox;
  BButton *_boldButton, *_italicButton;
  BText *_preview;
  BButton *_okButton, *_cancelButton;
  bool _cancelled;
  BFont *_font;
};

#endif /* BFONTSELECTOR_H */

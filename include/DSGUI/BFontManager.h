// BFontManager.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BFONTMANAGER_H
#define BFONTMANAGER_H

/* system includes */
/* (none) */

/* my includes */
#include "BFont.h"

/*!
  \brief Font management master class (singleton)

  You cannot create BFontManager instances yourself. There is only one
  instance, accessed using the get() method.
*/
class BFontManager {
public:
  static BFontManager* get();
  
  typedef struct {
    std::string name;
    bool bold;
    bool oblique;
    int height;
    BFont* font;
    std::string filename;
  } FontDescription;

  void addFont(BFont* font);
  void scanDirectory(const std::string& dir);

  int numFonts() { return _fonts.size(); }

  const std::vector<std::string>& fontNames() { return _names; }
  std::vector<int> sizesForFont(const std::string& name);
  bool hasFont(const std::string& name, int size, bool bold, bool oblique);
  BFont* font(const std::string& name, int size, bool bold, bool oblique);

protected:
  friend class BFont;
  void fontIsDeleted(BFont* font);
  
private:
  bool fontAlreadyIn(const FontDescription& descr);
  void addUniqueFontName(const std::string& name);
  
  std::vector<FontDescription> _fonts;
  std::vector<std::string> _names;
  static BFontManager* _fm;
};

#endif /* BFONTMANAGER_H */

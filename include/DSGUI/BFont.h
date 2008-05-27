// BFont.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BFONT_H
#define BFONT_H

/* system includes */
#include <nds.h>
#include <string>
#include <vector>
#include <string.h>

/* my includes */
#include "BImage.h"
#include "BWidget.h"
#include "BGraphics.h"
#include "BVirtualFile.h"

/*!
  \brief Antialiased, multi character range, bitmap font class

  A font is an 8-bit greyscale bitmap with all font characters laid
  out horizontally in a single row. The indexing method into the
  bitmap uses character ranges and arrays of character start and width
  in the bitmap. Example: A font with character ranges [31..32] and
  [65..100], if asked for character glyph 66, will use the fourth
  entry in the character start/width array to determine the bitmap
  position the glyph can be found at.

  Of the 8 bit, only the lowest 4 bits (0-31) are used.
*/
  
class BFont {
public:
  typedef struct {
    unsigned int start;
    unsigned int width;
  } GlyphDescription;

  typedef enum {
    FLAG_REGULAR = 0,
    FLAG_BOLD = 1,
    FLAG_OBLIQUE = 2
  } FontFlag;

  typedef struct {
    uint8 formatflags, encodingflags;
    std::string name, fullname;
    uint8 height;
    uint8 fontflags;
  } FontHeader;

  typedef struct {
    uint32 start, end;
  } CharacterRange;

  /*!
    \brief Read the font header from a font file

    This is mainly for inspection of the files in a directory. It
    allows examination of the header contents (name, height, format
    flags) without actually having to load the large font bitmap.
  */
  static bool readFontHeader(BVirtualFile& f, FontHeader& header);

  /*!
    \brief Create a font from an image and character range

    This allows only the use of a single char range; the glyph
    description array must be (lastchar-firstchar) entries long.
  */
  BFont(const BImage& img, const GlyphDescription* indices,
	int firstchar, int lastchar);

  /*!
    \brief Create a font by reading from the given file
  */
  BFont(BVirtualFile& f);
  ~BFont();

  //! Return the width of the given glyph
  int widthOfGlyph(uint32 glyph);
  //! Return the width of the given string (by adding its glyph widths)
  int widthOfString(const std::string& str);
  //! Return the font height
  int height() { return img->height(); }

  /*!
    \brief Draw a glyph at the given position

    Only the pixels of the glyph inside the clip rectangle are
    actually drawn. The text direction is actually a rotation. The
    point gives the top left pixel of the glyph, <i>in glyph
    coordinates</i>.

    The returned value is the bottom right pixel of the glyph and can
    be used as a cursor.
  */
  BPoint drawGlyph(BImage& img, const BPoint& point, uint32 glyph,
		   uint16 color, const BRect& clip,
		   BWidget::TextDirection dir = BWidget::DIR_LEFTTORIGHT);

  //! Draw a string (by calling drawGlyph() repeatedly)
  BPoint drawString(BImage& img, const BPoint& point, const std::string& str,
		    uint16 color, const BRect& clip,
		    BWidget::TextDirection dir = BWidget::DIR_LEFTTORIGHT);

  //! Return the font header
  const FontHeader& fontHeader() { return header; }

  /*!
    \brief Use the font manager to find the same font in a different size

    Delta gives a relative size change (so if this is a 10 pixel font,
    a delta of -2 would look for an 8 pixel font). If the requested
    font cannot be found, this is returned.
  */
  BFont* otherSizeFont(int delta);

private:
  BImage* img;

  std::vector<CharacterRange> ranges;
  std::vector<GlyphDescription> indices;
  
  FontHeader header;

  bool readFontBody(BVirtualFile& f, const FontHeader& header);

  static inline uint8 read8(BVirtualFile& f)
  {
    uint8 byte;
    f.read(&byte, 1);
    return byte;
  }
  
  static inline uint16 read16(BVirtualFile& f)
  {
    return read8(f) << 8 | read8(f);
  }

  static inline uint32 read32(BVirtualFile& f)
  {
    return read8(f) << 24 | read8(f) << 16 | read8(f) << 8 | read8(f);
  }

  static inline std::string readstring(BVirtualFile& f)
  {
    uint8 len = read8(f);
    char* buf = new char[len+1];
    memset(buf, 0, len+1);
    f.read(buf, len);
    std::string str = buf;
    delete buf;
    return str;
  }

  inline int indexOfGlyph(uint32 glyph)
  {
    int index = 0;
    for(unsigned int i=0; i<ranges.size(); i++)
      {
	if(glyph >= ranges[i].start && glyph <= ranges[i].end)
	  return index+(glyph-ranges[i].start);
	index += (ranges[i].end-ranges[i].start)+1;
      }
    return -1;
  }
};

#endif /* BFONT_H */

// BImage.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BIMAGE_H
#define BIMAGE_H

/* system includes */
#include <string>

/* my includes */
#include "BGraphics.h"

class BPoint;
class BRect;
class BFont;
class BWidget;

/*!
  \brief Image class

  Images are the final target of all graphics commands. Images can
  reside on the heap or be references to VRAM banks.

  Images are commonly 15bpp RGB images, with bit 15 indicating
  alpha. For some special purposes (font bitmaps, for example), images
  can also be 8bpp greyscale.
*/

class BImage {
public:

  typedef enum {
    FMT_RGB15,
    FMT_GREY5
  } ImageFormat; 

  /*!
    \brief Construct an image from an XPM-encoded data string

    See the XPM data format for details.
  */
  BImage(char** xpmData);

  /*!
    \brief Construct an image of the given size

    This will allocate as much data as needed.
  */
  BImage(int width, int height,
	 ImageFormat format = FMT_RGB15);

  /*!
    \brief Construct an image of the given size from the given data

    If copy is true, this will allocate enough data (and free the data
    when the image is deleted). If copy is false, this will reference
    the given data and not free it. Use, for example, with VRAM banks.
  */
  BImage(int width, int height,
	 uint8* image, ImageFormat format = FMT_RGB15,
	 bool copy = false);
  BImage(const BImage& img);

  virtual ~BImage();

  void operator=(const BImage& img);

  //! Return the image's width
  virtual int width() const { return _width; }
  //! Return the image's height
  virtual int height() const { return _height; }
  //! Return the image's format
  virtual int format() const { return _format; }

  /*!
    \brief Blit the data onto the given point

    If format is not FMT_RGB15, this does nothing currently.
  */
  virtual void blit(const BPoint& pt, uint16* screenStart);

  /*!
    \name Drawing commands

    These operate on RGB15 images only, currently. They also do not
    observe image boundaries, with the exception of drawText(). Use
    with caution! 

    \{
  */

  //! Draw a pixel at the given point
  void drawPixel(const BPoint& pt, uint16 color);
  //! Draw a pixel at the given point
  void drawPixel(int x, int y, uint16 color);
  //! Draw a line between the given points (not anti-aliased)
  void drawLine(const BPoint& pt1, const BPoint& pt2, uint16 color);
  //! Draw a rectangle outline
  void drawRect(const BRect& rect, uint16 color);
  //! Draw a section of text
  BPoint drawText(BFont* font, const std::string& text,
		  int cursorPos, const BRect& clip,
		  const BWidget& widget, uint16 color);

  /*!
    \}
  */

  //! Return the image storage
  uint8* bytes() { return _image; }
  //! Return the image storage
  const uint8* bytes() const { return _image; }

  //! Fill the (RGB15) image with the given color.
  void fill(uint16 color);

private:
  uint8* _image;
  bool _freeImage;
  int _width, _height;
  ImageFormat _format;
};

#endif /* BIMAGE_H */

/* This is -*- C -*-)

   BGraphics.h   
   \author: Bjoern Giesler <bjoern@giesler.de>
   
   
   
   $Author: giesler $
   $Locker$
   $Revision$
   $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $
 */

#ifndef BGRAPHICS_H
#define BGRAPHICS_H

/* system includes */
#include <nds.h>

/* my includes */
#include "BImage.h"

class BImage;

class BPoint {
 public:
  BPoint(int x=0, int y=0)
    {
      this->x = x; this->y = y;
    }

  BPoint operator+(const BPoint& pt) { return BPoint(x+pt.x, y+pt.y); }
  BPoint operator-(const BPoint& pt) { return BPoint(x-pt.x, y-pt.y); }
  int x, y;
};

class BSize {
 public:
  BSize(unsigned int width=0, unsigned int height=0)
    {
      this->width = width; this->height = height;
    }

  inline bool isZeroSize() { return width==0 || height==0; }
  int width, height;
};

class BLine {
 public:
  BLine(const BPoint& p0, const BPoint& p1)
    {
      this->p0 = p0; this->p1 = p1;
    }
  BLine(int x1, int y1, int x2, int y2)
    {
      BLine(BPoint(x1, y1), BPoint(x2, y2));
    }
    
  BPoint p0, p1;
  void draw(BImage& img, uint16 color) const;
};

class BRect {
 public:
  BRect(int x=0, int y=0, unsigned int width=0, unsigned int height=0);
  BRect(const BPoint& point, const BSize& size);
  BRect(const BPoint& p1, const BPoint& p2);
  
  BPoint pt;
  BSize sz;

  inline BPoint topLeft() const {
    return pt;
  }
  inline BPoint topRight() const {
    return BPoint(pt.x+sz.width, pt.y);
  }
  inline BPoint bottomLeft() const {
    return BPoint(pt.x, pt.y+sz.height);
  }
  inline BPoint bottomRight() const {
    return BPoint(pt.x+sz.width, pt.y+sz.height);
  }

  BRect insetRect(int left, int right, int top, int bottom) const;

  inline bool intersects(const BRect& r) const {
    return (pt.x + sz.width > r.pt.x && pt.x < r.pt.x + r.sz.width &&
	    pt.y + sz.height > r.pt.y && pt.y < r.pt.y + r.sz.height);
  }

  void outline(BImage& img, uint16 color) const;
  void fill(BImage& img, uint16 color) const;
  bool containsPoint(const BPoint& p) const;
};

inline uint16 alphablend(uint16 rgb1, uint16 rgb2, char alpha)
{
  alpha &= 0x1f;
  uint16 rb1 = rgb1 & 0x7c1f;
  uint16 rb2 = rgb2 & 0x7c1f;
  uint16 g1 = rgb1 & 0x3e0;
  uint16 g2 = rgb2 & 0x3e0;
  uint16 rbout = (rb2 + (((rb1-rb2)*alpha + 0x4010) >> 5)) & 0x7c1f;
  uint16 gout = (g2 + (((g1-g2)*alpha + 0x200) >> 5)) & 0x3e0;

  return rbout | gout | 0x8000;
}

#endif /* BGRAPHICS_H */

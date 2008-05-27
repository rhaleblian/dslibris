// BException.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BEXCEPTION_H
#define BEXCEPTION_H

/* system includes */
#include <string>

/* my includes */
#include "BStringUtils.h"

class BException {
public:
  BException(const std::string& file, int line, const std::string& txt);
  std::string description(bool msgonly = false);
protected:
  BException() {}
  std::string file;
  int line;
  std::string text;
};

class BSystemException: public BException {
public:
  BSystemException(const std::string& file, int line, const std::string& txt);
};

#define BTHROW(cls, text, ...) throw cls(__FILE__, __LINE__, format(text))
  
#endif /* BEXCEPTION_H */

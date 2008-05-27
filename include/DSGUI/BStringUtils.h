// BStringUtils.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BSTRINGUTILS_H
#define BSTRINGUTILS_H

/* system includes */
#include <string>
#include <deque>

/* my includes */
/* (none) */

std::string strip(const std::string& str, bool asciiOnly = false);
std::deque<std::string> split(const std::string& str,
			      const std::string& separator);
std::string toUpper(const std::string& str);
std::string toLower(const std::string& str);
std::string format(const std::string& fmt, ...);

bool startsWith(const std::string& str, const std::string& prefix);
bool endsWith(const std::string& str, const std::string& suffix);

#endif /* BSTRINGUTILS_H */

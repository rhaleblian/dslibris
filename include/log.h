#ifndef _LOG_H_
#define _LOG_H_

/*
 * log.h
 *
 *  Created on: Jul 11, 2009
 *      Author: rhaleblian
 */

#include <string>

void Log(const char *msg);
void Log(std::string msg);
void Log(const char *format, const char *msg);

#endif

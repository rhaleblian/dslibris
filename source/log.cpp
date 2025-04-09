/*
 * log.cpp
 *
 *  Created on: Jul 11, 2009
 *      Author: rhaleblian
 */
#include <sstream>

#include "log.h"

void Log(const char *format, const char *msg)
{
	FILE *logfile = fopen(LOGFILEPATH, "a");
	if (logfile) {
		fprintf(logfile,format,msg);
		fclose(logfile);	
	}
}

void Log(const char *msg)
{
	Log("%s", msg);
}

void Log(std::string msg)
{
	Log("%s", msg.c_str());
}

void Log(const char *format, const int value)
{
	std::stringstream ss;
	ss << value << std::endl;
	Log(format, ss.str().c_str());
}

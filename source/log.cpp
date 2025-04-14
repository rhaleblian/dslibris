/*
 * log.cpp
 *
 *  Created on: Jul 11, 2009
 *      Author: rhaleblian
 */

#include <nds.h>
#include <sstream>

#include "define.h"
#include "log.h"

void Log(const char *msg)
{
	Log("%s", msg);
}

void Log(std::string msg)
{
	Log("%s", msg.c_str());
}

void Log(const char *format, const char *msg)
{
	// if(console)
	// {
	// 	char s[1024];
	// 	sprintf(s,format,msg);
	// 	iprintf(s);
	// }
	// FILE *logfile = fopen(LOGFILEPATH,"a");
	// fprintf(logfile,format,msg);
	// fclose(logfile);
	FILE *logfile = fopen(LOGFILEPATH, "a");
	fprintf(logfile,format,msg);
	fclose(logfile);
}

void Log(const char *format, const int value)
{
	std::stringstream ss;
	ss << value << std::endl;
	Log(format, ss.str().c_str());
}

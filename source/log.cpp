/*
 * log.cpp
 *
 *  Created on: Jul 11, 2009
 *      Author: rhaleblian
 */
#include <stdio.h>
#include "define.h"
#include "log.h"

int level = LOG_LEVEL_INFO;

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
	FILE *logfile = fopen(LOGFILEPATH, "a");
	fprintf(logfile,format,msg);
	fclose(logfile);
}

int SetLevel(int _level) {
	int previous_level = level;
	level = _level;
	return previous_level;
}

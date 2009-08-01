/*
 * log.cpp
 *
 *  Created on: Jul 11, 2009
 *      Author: rhaleblian
 */
#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include "log.h"

#define LOGFILEPATH "dslibris.log"

void Log(const char *msg)
{
	Log("%s",msg);
}

void Log(std::string msg)
{
	Log("%s",msg.c_str());
}

void Log(const char *format, const char *msg)
{
	FILE *logfile = fopen(LOGFILEPATH,"a");
	fprintf(logfile,format,msg);
	fclose(logfile);
}

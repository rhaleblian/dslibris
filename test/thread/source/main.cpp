/* 

Simple C++11 threading tests.

 Copyright (C) 2020 Ray Haleblian (ray@haleblian.com)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include <thread>
//#include <stdio.h>
#include <iostream>
#include "nds.h"

PrintConsole* boot_console(void) {
	// Get a console going.
	auto console = consoleDemoInit();
	if (!console) iprintf("[FAIL] console!\n");		// This, of course, won't print :D
	else iprintf("[OK] console\n");
	return console;
}

void spin(void) {
	while(true) swiWaitForVBlank();
}

void fatal(const char *msg) {
	iprintf(msg);
	spin();
}

void thread_function()
{
    for(int i = 0; i < 10000; i++);
    iprintf("thread function Executing\n");
}

#if HAVE_THREAD
int main_()
{
    
    std::thread threadObj(thread_function);
    for(int i = 0; i < 10000; i++);
        iprintf("Display From MainThread\n");
    threadObj.join();    
    iprintf("Exit of Main function\n");
    return 0;
}
#endif

int main(void)
{
	if(!boot_console()) spin();
#if HAVE_THREAD
	main_();
#else
	iprintf("er, have you thread support?\n");
#endif
	while(true) swiWaitForVBlank();
}

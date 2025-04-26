/* 

dslibris - an ebook reader for the Nintendo DS.

 Copyright (C) 2007-2025 Ray Haleblian (ray@haleblian.com)

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

#include <nds.h>
#include "app.h"

int main(int argc, char **argv) {
	//! Main entry point for dslibris.
	// defaultExceptionHandler();
	// consoleDebugInit(DebugDevice_NOCASH);
	// glInit();
	App *app = new App();
	app->Init();
	return app->Run();
}

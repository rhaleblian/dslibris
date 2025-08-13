/* 

dslibris - an ebook reader for the Nintendo DS.

 Copyright (C) 2007-2025 Yoyodyne Research ZLC

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

#include "main.h"

#include "dirent.h"
#include "fat.h"
#include "nds.h"

#include "app.h"
#include "book.h"
#include "button.h"
#include "text.h"
#include "expat.h"
#include "parse.h"
#include "version.h"

App *app;
char msg[256];

//! \param vblanks blanking intervals to wait, -1 for forever, default = -1
int halt(int vblanks) {
	int timer = vblanks;
	while(pmMainLoop()) {
		swiWaitForVBlank();
		if (timer == 0) break;
		else if (timer > 0) timer--;
	}
	return 1;
}

//! \param vblanks blanking intervals to wait, -1 for forever, default = -1
int halt(const char *msg, int vblanks) {
	consoleDemoInit();
	printf(msg);
	return halt(vblanks);
}

int main(void)
{
	defaultExceptionHandler();
	// consoleDemoInit();
	// consoleDebugInit(DebugDevice_NOCASH);
	// fprintf(stderr, "dslibris %s\n", VERSION);

	if (!fatInitDefault())
		halt("[FAIL] filesystem\n");

	app = new App();
	return app->Run();
}

u8 GetParserFace(parsedata_t *pdata)
{
	if (pdata->italic)
		return TEXT_STYLE_ITALIC;
	else if (pdata->bold)
		return TEXT_STYLE_BOLD;
	else
		return TEXT_STYLE_REGULAR;
}

void WriteBufferToCache(parsedata_t *pdata)
{
	// Only cache if we are laying out text.
	//	if (pdata->cachefile && pdata->status) {
	//		fwrite(pdata->buf, 1, pdata->buflen, pdata->cachefile);
	//}
}

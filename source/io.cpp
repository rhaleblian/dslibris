
#if 0
#include <nds.h>

#include "dirent.h"
#include "fat.h"

PrintConsole* boot_console(void) {
	// Get a console going.
	auto console = consoleDemoInit();
	if (!console) return NULL;
	else iprintf("\n[ OK ] dslibrOS 1.5\n");
	return console;
}

int boot_filesystem(void) {
	// Start up the filesystem.
	bool success = fatInitDefault();
	if (!success) iprintf("[FAIL] cannot start filesystem\n");
	else iprintf("[ OK ] started filesystem\n");
	return success;
}

int check_filesystem(void) {
	DIR *dp = opendir("/");
	if (!dp) {
		iprintf("[FAIL] no rootdir\n");
		swiWaitForVBlank();
		return false;
	}

	iprintf("[ OK ] opened rootdir\n");
	struct dirent *ent;
	while ((ent = readdir(dp)))
	{
		iprintf("%s %d\n", ent->d_name, ent->d_type);
	}
	closedir(dp);
	iprintf("[ OK ] closed rootdir\n");
	swiWaitForVBlank();

	return true;
}
#endif
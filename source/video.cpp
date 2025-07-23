#include <cstring>
#include "nds.h"

#include "splash.h"

int getSize(u8 *source, u16 *dest, u32 arg) {
       return *(u32*)source;
}

u8 readByte(u8 *source) { return *source; }

void drawstack(u16 *screen) {
       TDecompressionStream decomp = {getSize, NULL, readByte};
       swiDecompressLZSSVram((void*)splashBitmap, screen, 0, &decomp);
}

void CopyScreen(u16 *src, u16 *dst) {
	std::memcpy(src, dst, 256 * 256 * sizeof(u16));
}

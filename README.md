Herein lies the source for *dslibris*, an [EPUB](http://idpf.org/epub)
ebook reader for the Nintendo DS.

![Startup Screen](etc/browser.jpg)
![A Sample (Left and Right) Page](etc/page.jpg)

![UTF-8 Multingual](http://rhaleblian.files.wordpress.com/2007/09/utf8.png)

# Releases

See the Releases section for a program ready to be DLDI patched and copied to your cartridge media.
The release file also contains a file structure for books and fonts that you should also copy.

# Installation

1. DLDI patch `dslibris.nds`, if your cart needs it, and copy it to the root of your cart.
2. make a directory `font` and copy the fonts in it.
3. make a directory `book` and copy your books into it.

These directory locations cannot be changed.

For additional notes, see `INSTALL`.

# Development

## Prerequisites

devkitPro pacman packages:

    (dkp-)pacman -S devkitARM libnds libfat libfilesystem nds-expat nds-bzip nds-zlib nds-libpng nds-freetype

Ubuntu 16.04 LTS and macOS are known good development platforms.
CentOS and MinGW have also worked, but haven't been checked recently.

## Building

To build the program,

```shell
make
```

`dslibris.nds` should show up in the top directory.

## Debugging

arm-eabi-gdb, insight-6.8 and desmume-0.9.12-svn5575 have been known to work for debugging.
See online forums for means to build an arm-eabi-targeted Insight for your platform.

# See Also

https://rhaleblian.wordpress.com/dslibris-an-ebook-reader-for-the-nintendo-ds/

http://devkitpro.org

http://idpf.org/epub

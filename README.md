Herein lies the source for **dslibris**, an [EPUB](http://idpf.org/epub)
ebook reader for the Nintendo DS, DSi and DSi XL.

![Browser](etc/sample/browser.png)
![Faust](etc/smaple/faust.jpg)

![UTF-8 Multingual](http://rhaleblian.files.wordpress.com/2007/09/utf8.png)

# Releases

See the Releases section for a program ready to be DLDI patched and copied to your cartridge media.
The release file also contains a file structure for books and fonts that you should also copy.

# Installation

See ![INSTALL.txt](INSTALL.txt) .

# Development

## Prerequisites

devkitPro pacman packages:

    (dkp-)pacman -S devkitARM libnds libfat libfilesystem nds-expat nds-bzip2 nds-zlib nds-freetype nds-libpng nds-pkg-config ndstool dstools grit

Development is biased towards Ubuntu 20 as a platform.
You should also get far with macOS.
CentOS and msys2 have also worked, but haven't been checked recently.
Ubuntu under WSL would work too, but you'll be missing mount support for testing.

## Building

To build the program,

```shell
make
```

`dslibris.nds` should show up in the top directory.

## Debugging

arm-eabi-gdb, insight-6.8 and desmume-0.9.12-svn5575 have been known to work for debugging.
See online forums for means to build an arm-eabi-targeted Insight for your platform.

## Troubleshooting

### The app runs, but no text is rendered.

Instead of installing `nds-freetype`, build version 2.5.5 from source,
configuring it with

    portlibs/configure-freetype

instead of the supplied configure script. This will get inserted into the build
via the `build/` directory.

# See Also

https://rhaleblian.wordpress.com/dslibris-an-ebook-reader-for-the-nintendo-ds/

http://devkitpro.org

http://idpf.org/epub

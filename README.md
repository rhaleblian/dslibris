[![C/C++ CI](https://github.com/rhaleblian/dslibris/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/rhaleblian/dslibris/actions/workflows/c-cpp.yml)

Herein lies the source code for **dslibris**, an
[EPUB](https://en.wikipedia.org/wiki/EPUB)
ebook reader for the Nintendo DS, DSi and DSi XL.

![Browser](etc/sample/browser.png)

![Quickstart](etc/sample/quickstart.png)

![A sample page](etc/sample/iliad.png)


# Releases

See the Releases section for a program ready to be
[DLDI](https://wiki.gbatemp.net/wiki/DLDI)
patched and copied to your cartridge media.
The release file also contains a file structure for
books and fonts that you should also copy.


# Installation

See INSTALL.txt or the
[Quickstart](https://github.com/rhaleblian/dslibris/wiki/User:-Quickstart)
page in the Wiki.


# Development

## Prerequisites

We use devkitPro's toolchain for ARM, aka `devkitARM`.

Development is biased towards Debian-clan (eg Ubuntu) as a platform.
You should also get far with macOS.
CentOS and msys2 have also worked, but haven't been checked recently.
Ubuntu under WSL would work too, but you'll be missing mount support
for emulator testing.

The `bootstrap` script can speed up getting started after cloning the repo.

Some configuration exists for using Visual Studio Code.

## Building

To build the program, assure devkitARM is available to your shell:

    . activate

then

    make

`dslibris.nds` should show up in the top directory.

See the Makefile for rules that apply DLDI for a few specific cases
(R4, CycloDS Evolution, MPCF).

## Debugging

`$DEVKITARM/bin/arm-none-eabi-gdb` and `DeSMuME`,
using the latter's cflash image and ARM9 stub
features.

See the `gdb` Make rule for a debugging process.


# See Also

http://ray.haleblian.com/wordpress/dslibris-an-ebook-reader-for-the-nintendo-ds/

http://devkitpro.org

http://idpf.org/epub


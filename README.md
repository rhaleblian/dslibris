[![C/C++ CI](https://github.com/rhaleblian/dslibris/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/rhaleblian/dslibris/actions/workflows/c-cpp.yml)

Herein lies the source for **dslibris**, an [EPUB](https://en.wikipedia.org/wiki/EPUB)
ebook reader for the Nintendo DS, DSi and DSi XL.

![Browser](etc/sample/browser.png)

![Quickstart](etc/sample/quickstart.png)

![Faust](etc/sample/iliad.png)

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

Development is biased towards Ubuntu 20 as a platform.
You should also get far with macOS.
CentOS and msys2 have also worked, but haven't been checked recently.
Ubuntu under WSL would work too, but you'll be missing mount support for emulator testing.

The `bootstrap` script can speed up getting started.
Run it after cloning the repo.

Some configuration exists for using Visual Studio Code.

## Building

To build the program,

```shell
make
```

`dslibris.nds` should show up in the top directory.

## Debugging

`$DEVKITARM/bin/arm-none-eabi-gdb` and DeSMuME,
using the latter's cflash image and ARM9 stub
features.

See the `gdb` Make rule for a debugging process.


# See Also

http://devkitpro.org

http://idpf.org/epub

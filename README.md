[![C/C++ CI](https://github.com/rhaleblian/dslibris/actions/workflows/build.yml/badge.svg)](https://github.com/rhaleblian/dslibris/actions/workflows/build.yml)

Herein lies the source code for **dslibris**, an
[EPUB](https://en.wikipedia.org/wiki/EPUB)
ebook reader for the Nintendo DS, DSi and DSi XL.

![Browser](etc/sample/browser.png)

![Quickstart](etc/sample/quickstart.png)

![A sample page](etc/sample/iliad.png)

# Releases

Download the zip file in the Releases section.

# Installation

See INSTALL.txt or the
[Quickstart](https://github.com/rhaleblian/dslibris/wiki/User:-Quickstart)
page in the Wiki.

# Development

WARNING: the `main` branch currently fatally fails to render text.
dslibris will freeze in the book browser with no text displayed.

## Prerequisites

We use devkitPro's toolchain for ARM, aka `devkitARM`.

Development is biased towards Debian-clan (eg Ubuntu) as a platform.
You should also get far with macOS.
CentOS and msys2 have also worked, but haven't been checked recently.
Ubuntu under WSL would work too, but you'll be missing mount support
for emulator testing.

Some configuration exists for using Visual Studio Code.

Once `dkp-pacman` is installed, can get needed packages via

    sudo dkp-pacman -S $(cat requirements.txt)

Afterwards we need to install the legacy version of freetype that
dslibris relies on.

    sudo ./lockfix/setup-noinline
    ./install-freetype 2.4.8
    sudo ./lockfix/revert-inline

## Building

To build the program, assure devkitARM is available to your shell:

    . /opt/devkitpro/devkitarm.sh

then

    make

`dslibris.nds` should show up in the top directory.

## DLDI

If your media does not auto-patch DLDI, .dldi for a few specific cases
(R4, CycloDS Evolution, MPCF) are here:

    etc/dldi

used like e.g.

    dlditool etc/dldi/r4_v2.dldi dslibris.nds

## Debugging

Copy your build to a DS and maybe `Log()` messages.  Good luck.

# See Also

Blog: <http://ray.haleblian.com/wordpress/dslibris-an-ebook-reader-for-the-nintendo-ds/>

Development Tools: <http://devkitpro.org>

EPUB/W3C/IDPF: [http://idpf.org](https://idpf.org/)

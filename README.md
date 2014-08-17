Herein lies the source for dslibris, an EPUB reader for the Nintendo DS.

# Prerequisites
Fedora, Ubuntu, Arch Linux, OS X, and Windows XP have all been used as build platforms. Have:

*   devkitARM_r42
*   libnds-1.5.8
*   libfat-1.0.12
*   maxmod-nds-1.0.9
*   expat-2.1.0
*   freetype-2.4.8
*   zlib-1.2.8
*   a media card and a DLDI patcher, but you knew that.

To build expat, freetype and zlib for the ARM EABI architecture, obtain the source tarball and see etc/configure-* for working configure incantations. You will want the devkitARM GCC tools in your PATH:

> export PATH=$DEVKITARM/bin:$PATH

> make
> make install

will install into $DEVKITARM.

# Building
> make

dslibris.nds should show up in your current directory.
for a debugging build,

> DEBUG=1 make

# Installation
See INSTALL.

# Debugging
arm-eabi-gdb, insight-6.8 and desmume-0.9.2 have been known to work for debugging. See online forums for means to build an arm-eabi-targeted Insight for your platform.

# More Info
http://sourceforge.net/projects/ndslibris

ray haleblian

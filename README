
Herein lie the source for dslibris, an EPUB reader for the Nintendo DS.

= PREREQUISITES

Fedora, Ubuntu, Arch Linux, OS X, and Windows XP have all been used as build
platforms. Have:

* devkitPro circa 2011-10-15: 
  * default_arm7-0.5.23
  * devkitARM_r35
  * libfat-1.0.10
  * libnds-1.5.4
  * maxmod-1.0.6
* expat-2.1.0, freetype-2.4.3, and zlib-1.2.8, built for ARM EABI.
* a media card and a DLDI patcher, but you knew that.

To build expat, freetype and zlib, obtain the source tarball and see 
etc/configure-* for working configure incantations. zlib may require modifying
the Makefile for cross-compiling. You may need to get the GCC tools in your PATH:

  export PATH=$PATH:$DEVKITARM/bin

Installation is into $DEVKITARM.

= BUILDING

  make

dslibris.nds should show up in your current directory.
for a debugging build,

  DEBUG=1 make

= INSTALLATION

see INSTALL.

= DEBUGGING

arm-eabi-gdb, insight-6.8 and desmume-0.9.2 have been known to work for
debugging. See online forums for means to build an arm-eabi-targeted
Insight for your platform.

= MORE INFO

http://sourceforge.net/projects/ndslibris

- ray haleblian


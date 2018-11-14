Herein lies the source for *dslibris*, an EPUB reader for the Nintendo DS.

# Prerequisites

Ubuntu 16.04 LTS is known to work as a build platform.
Other platforms might, as long as the correct toolchain can be obtained.

*   devkitARM r43
*   libnds-1.5.8
*   libfat-1.0.12
*   expat-2.1.0
*   freetype-2.4.8
*   zlib-1.2.8
*   a media card and a DLDI patcher, but you knew that.

devkitARM r43 is not easily found. If you need to build it,
clone the r43 branch of devkitPro/buildscripts from GitHub and patch it with the files in tool/buildscripts. Use the installer tarballs there if the source locations have disappeared.

After installing devkitPro/devkitARM, see

> etc/profile

for an example of setting DEVKITPRO and DEVKITARM in your shell.

# Building

To build the dependent libraries,

> cd tool
> make

The arm-none-eabi-* executables must be in your PATH for the above to work.

Then, to build the program,

> cd ..
> make

dslibris.nds should show up in your current directory.
for a debugging build,

> DEBUG=1 make


# Installation

See INSTALL.

# Screenshots

![UTF-8 Multingual](http://rhaleblian.files.wordpress.com/2007/09/utf8.png)

# Debugging

arm-eabi-gdb, insight-6.8 and desmume-0.9.12-svn5575 have been known to work for debugging. See online forums for means to build an arm-eabi-targeted Insight for your platform.

# More Info

http://github.com/rhaleblian/dslibris

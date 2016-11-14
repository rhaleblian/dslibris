Herein lies the source for dslibris, an EPUB reader for the Nintendo DS.

# Prerequisites
CentOS, Fedora, Ubuntu, Arch Linux, OS X, and Windows XP have all been used as build platforms. Needs:

*   devkitPro r42 with devkitARM
*   libnds-1.5.9
*   libfat-nds-1.0.13
*   maxmod-nds-1.0.9
*   expat-2.1.0
*   freetype-2.4.8
*   zlib-1.2.8
*   a media card and a DLDI patcher, but you knew that.

After installing devkitPro/devkitARM, see

> etc/profile

for an example of setting DEVKITPRO and DEVKITARM in your shell. based on where you installed devkitPro.

Source tarballs for the above are in tool/. See the Makefile 'install-tools' rule for where all of the devkitPro bits need to go. See etc/configure-* for configure incantations. 


# Building
> make

dslibris.nds should show up in your current directory.
for a debugging build,

> DEBUG=1 make


# Installation
See INSTALL.


# Debugging
arm-eabi-gdb, insight-6.8 and desmume-0.9.12-svn5575 have been known to work for debugging. See online forums for means to build an arm-eabi-targeted Insight for your platform.

# More Info
http://sourceforge.net/projects/ndslibris

ray haleblian

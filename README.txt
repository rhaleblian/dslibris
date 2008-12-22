
Herein lie the source for dslibris, and supporting libraries.

PREREQUISITES

Fedora, Ubuntu, Arch, OS X, and Windows XP have all been used as build platforms; currently we use Ubuntu 8.04 and Arch Linux. Have:

* devkitPro (circa r20) installed with components:
	devkitARM
	libnds
	libfat
	libwifi
	masscat's DS wifi debug stub library
  Set DEVKITPRO and DEVKITARM set in your environment.
* on Windows XP, MSYS/MINGW. The MSYS provided with devkitPro is fine.
* optionally, desmume 0.7.3 or later if you want to debug with gdb.
* a media card and a DLDI patcher, but you knew that.

BUILDING

cd ndslibris/trunk  # or wherever you put the SVN trunk
make

dslibris.nds should show up in your current directory.
 
Note the support libraries in 'lib', prebuilt for arm-eabi; make sure you don't have conflicting libs in your path.

INSTALLATION

see INSTALL.txt.

DEBUGGING

gdb and insight-6.6 have been known to work for debugging. See online forums for means to build an arm-eabi targeted Insight for your platform.

MORE INFO

http://sourceforge.net/projects/ndslibris

- ray haleblian


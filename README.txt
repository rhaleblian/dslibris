
Herein lie the source for dslibris, and supporting libraries.

PREREQUISITES

Fedora, Ubuntu, Arch, OS X, and Windows XP have all been used as build platforms; currently I use Ubuntu 9.04. Have:

* devkitPro from late October 2007, including:
	devkitARM-23b
	libnds-20071023
	libfat-nds-20070127
	dswifi-0.3.3b
* on Windows XP, MSYS/MINGW. The MSYS provided with devkitPro is fine.
* a media card and a DLDI patcher, but you knew that.

BUILDING

cd ndslibris/trunk  # or wherever you put the SVN trunk
make

dslibris.nds should show up in your current directory.
 
Note the support libraries in 'lib', prebuilt for arm-eabi; make sure you don't have conflicting libs in your path.

INSTALLATION

see INSTALL.txt.

DEBUGGING

arm-eabi-gdb and insight-6.8 and desmume-0.7.2 have been known to work for debugging. See online forums for means to build an arm-eabi targeted Insight for your platform.

MORE INFO

http://sourceforge.net/projects/ndslibris

- ray haleblian


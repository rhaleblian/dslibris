
Herein lie the source for dslibris, and supporting libraries.

PREREQUISITES

Fedora, Ubuntu, Arch, OS X, and Windows XP have all been used as build platforms; currently it's OS X.

* devkitPro installed with components:
	devkitARM (r24)
	libnds
	libfat
	libwifi
  Set DEVKITPRO and DEVKITARM set in your environment.
* on Windows XP, MSYS/MINGW. The MSYS provided with devkitPro is fine.
* a media card and a DLDI patcher, but you knew that.

BUILDING

cd ndslibris/trunk  # or wherever you put the SVN trunk
make

dslibris.nds should show up in your current directory.
 
Note the libraries in 'external', prebuilt for arm-eabi; make sure you don't have conflicting libs in your path.

INSTALLATION

see INSTALL.txt.

DEBUGGING

gdb/insight-6.6 have been known to work on Ubuntu 8.04 for debugging; however CFLASH FAT support has been up and down. You can use the --cflash option to desmume to create a disk image.

See online forums for means to build an arm-eabi targeted Insight for your platform, and on making a FAT CFLASH image (keywords: fat desmume chism).

MORE INFO

For code documentation:

	cd $where_you_put_source
	doxygen	
	firefox doc/html/index.html

Also see:

http://sourceforge.net/projects/ndslibris

- ray haleblian


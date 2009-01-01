
Herein lie the source for dslibris, and supporting libraries.

== PREREQUISITES ==

Fedora, Ubuntu, Arch, OS X, and Windows XP (MSYS) have all been used as build platforms. Currently it's OS X 10.5.6. You need

* devkitPro r24 installed with devkitARM, libnds, libfat, and libwifi.
* A media card and a DLDI patcher, but you knew that.

== BUILDING ==

  cd ndslibris/trunk  # or wherever you put the SVN trunk
  make

dslibris.nds should show up in your current directory.
 
Note the libraries in 'external', prebuilt for arm-eabi; make sure you don't have conflicting libs in your path.

== INSTALLATION ==

See INSTALL.txt.

== DEBUGGING ==

gdb and insight-6.6 have been known to work on Ubuntu 8.04 for debugging; however CFLASH FAT support has been up and down. See the Makefile for invocation examples. See online forums for means to build an arm-eabi targeted insight for your platform, and on making a FAT CFLASH image (keywords: fat desmume chism).

== MORE DOCUMENTATION ==

For code documentation:

	cd $where_you_put_source
	doxygen	
	firefox doc/html/index.html

Also see:

http://sourceforge.net/projects/ndslibris

- rayh23


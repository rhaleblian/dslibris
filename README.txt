Herein lie dslibris and supporting libraries.
The method below assumes you are using MSYS/MINGW with devkitPro.

 To build:

1. get the files

export CVSROOT=:ext:eris.kicks-ass.net:/usr/local/cvsroot
export CVS_RSH=ssh
cvs co nds/dslibris
cvs co nds/expat
cvs co nds/freetype

You may need to install MINGW to get cvs.

2. configure and make expat and freetype

cd nds/expat
./arm-configure
make
make install

cd ../../nds/freetype
./arm-configure
make
make install

The libs will be installed in $DEVKITARM/lib and $DEVKITARM/include.

3. make dslibris

cd ../../nds/dslibris
make


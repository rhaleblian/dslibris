
Herein lie dslibris and supporting libraries.

BUILDING

Fedora, Ubuntu, OS X, and Windows XP have all been used as build platforms. The primary build platform is Fedora.

Prerequisites:

* devkitPro
* If on Windows XP, MSYS/MINGW. The MSYS provided with devkitPro is fine.
* DEVKITPRO and DEVKITARM set in your environment.

To build:

cd ndslibris/trunk
make

dslibris.nds is your target, and shows up at the root level.
 
Note the libraries in 'external', prebuilt for arm-eabi; make sure you don't have conflicting libs in your path.

INSTALLATION

Copy dslibris.nds and dslibris.ttf to the root of your media.

DEBUGGING

gdb and Insight have been known to work for debugging. See online forums for
means to build an arm-eabi gdb for you platform.


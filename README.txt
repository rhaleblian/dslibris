
Herein lie dslibris and supporting libraries. The method below assumes you are using either MSYS/MINGW with devkitPro.

BUILDING

Fedora, Ubuntu, OS X, and Windows XP have all been used as build platforms. The primary build platform is Windows XP.

Prerequisites:

* devkitPro
* If on Windows XP, MSYS/MINGW. The MSYS provided with devkitPro is fine.

To build:

cd ndslibris/trunk
make

dslibris.nds is your target, and shows up at the root level.
 
Note the libraries in 'external', prebuilt for arm-eabi; make sure you don't have conflicting libs in your path.

INSTALLATION

see INSTALL.txt.

DEBUGGING

Use insight with DeSmuME on XP and Linux. DeSmuME lacks the proper support to run or debug on OS X.


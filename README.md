Herein lies the source for *dslibris*, an [EPUB](http://idpf.org/epub)
ebook reader for the Nintendo DS.

![Startup Screen](etc/2.jpeg)
![A Sample (Left and Right) Page](etc/2-2.jpeg)

![UTF-8 Multingual](http://rhaleblian.files.wordpress.com/2007/09/utf8.png)

# Prerequisites

devkitPro pacman packages:

    dkp-pacman -U devkitARM libnds libfat nds-expat nds-bzip nds-zlib

Ubuntu 16.04 LTS and macOS are known good development platforms. CentOS and Cygwin have also worked, but haven't been checked recently.

# Building

In order for Freetype to build, the `arm-none-eabi-*` executables must be in your `PATH`.

Then, to build the program,

```shell
make
```

`dslibris.nds` should show up in the top directory.

# Installation

See INSTALL.

# Debugging

arm-eabi-gdb, insight-6.8 and desmume-0.9.12-svn5575 have been known to work for debugging. See online forums for means to build an arm-eabi-targeted Insight for your platform.

# More Info

https://rhaleblian.wordpress.com/dslibris-an-ebook-reader-for-the-nintendo-ds/

http://devkitpro.org

---
[1] http://idpf.org/epub

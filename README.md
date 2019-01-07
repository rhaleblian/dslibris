Herein lies the source for *dslibris*, an [EPUB](http://idpf.org/epub)
ebook reader for the Nintendo DS.

![Startup Screen](etc/2.jpeg)
![A Sample (Left and Right) Page](etc/2-2.jpeg)

![UTF-8 Multingual](http://rhaleblian.files.wordpress.com/2007/09/utf8.png)

# Prerequisites

Ubuntu 16.04 LTS is Ray's current development platform.
macOS, CentOS and Cygwin have also worked, but haven't been checked recently.

*   devkitARM r45
*   libnds-1.5.8
*   libfat-1.0.12
*   a media card and a DLDI patcher, but you knew that.

# Building

To build the dependent libraries,

```shell
cd tool
make
```

The arm-none-eabi-* executables must be in your PATH for the above to work.

Then, to build the program,

```shell
cd ..
make
```

dslibris.nds should show up in your current directory.
for a debugging build,

```shell
DEBUG=1 make
```

# Installation

See INSTALL.

# Debugging

arm-eabi-gdb, insight-6.8 and desmume-0.9.12-svn5575 have been known to work for debugging. See online forums for means to build an arm-eabi-targeted Insight for your platform.

# More Info

http://github.com/rhaleblian/dslibris


[1] http://idpf.org/epub

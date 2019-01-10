FREETYPE_VERSION=2.5.5
TARGETS=lib/libfreetype.a
PATH+=:${DEVKITARM}/bin

default: ${TARGETS}

clean:
	- rm -r bin include lib share
	- rm -r freetype-${FREETYPE_VERSION}

lib/libfreetype.a: freetype-${FREETYPE_VERSION}
	cd $< && (../configure-freetype && make install)

freetype-${FREETYPE_VERSION}:
	tar xf $@.tar.bz2

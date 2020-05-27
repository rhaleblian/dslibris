PATH+=:${DEVKITARM}/bin

TARGETS=lib/libfreetype.a

default: ${TARGETS}

clean:
	- rm -r bin include lib share
	- make -C freetype2 clean

VERSION=VER-2-10-0
DIR=freetype2-$(VERSION)
ARCHIVE=$(VERSION).tar.gz
URL=https://github.com/aseprite/freetype2/archive/VER-2-10-0.tar.gz

PATH:=${DEVKITARM}/bin:$(PATH)

install: build
	make -C $(DIR) install

build: configure
	# CMake isn't usable until cross-compiling switches exist.
	#cd $< && install -d build && cd build && cmake .. && make
	make -C $(DIR)

configure: $(DIR)
	cd $< && ./autogen.sh && ../configure-arm

$(DIR): $(ARCHIVE)
	tar xf $<

$(ARCHIVE):
	wget $(URL)

clean:
	make -C $(DIR) clean

distclean:
	make -C $(DIR) distclean
	- rm $(ARCHIVE)
	- rm -rf $(DIR)

preflight:
	apt install libtool

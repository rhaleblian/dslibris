VERSION=2.2.9
DIR=expat-$(VERSION)
ARCHIVE=$(DIR).tar.gz
URL=https://github.com/libexpat/libexpat/releases/download/R_2_2_9/expat-2.2.9.tar.gz

PATH:=${DEVKITARM}/bin:$(PATH)

install: build
	make -C $(DIR) install

build: configure
	make -j3 -C $(DIR)

configure: $(DIR)
	cd $< && ../configure-arm

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
	true

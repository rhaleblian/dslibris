VERSION=1.6.35
DIR=libpng-$(VERSION)
ARCHIVE=v$(VERSION).tar.gz
URL=https://github.com/glennrp/libpng/archive/v1.6.35.tar.gz

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

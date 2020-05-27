VERSION=1.2.11
DIR=zlib-$(VERSION)
ARCHIVE=v$(VERSION).tar.gz
URL=https://github.com/madler/zlib/archive/v1.2.11.tar.gz

PATH:=${DEVKITARM}/bin:$(PATH)

install: build
	make -C $(DIR) install

build: configure
	make -C $(DIR)

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

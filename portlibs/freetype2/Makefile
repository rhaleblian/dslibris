VERSION=VER-2-10-0
DIR=freetype2-$(VERSION)
ARCHIVE=$(DIR).tar.gz
URL=https://github.com/aseprite/freetype2/archive/$(ARCHIVE)

PATH:=${DEVKITARM}/bin:$(PATH)

freetype2: $(DIR)
	#cd $< && install -d build && cd build && cmake .. && make
	cd $< && ./autogen.sh && ../configure && make install
	
$(DIR): $(ARCHIVE)
	tar xf $<

$(ARCHIVE):
	curl $(URL)

preflight:
	apt install libtool

clean:
	- rm $(ARCHIVE)
	- rm -rf $(DIR)

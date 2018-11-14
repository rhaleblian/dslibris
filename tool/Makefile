TARGETS=lib/libexpat.a lib/libz.a lib/libfreetype.a

default: ${TARGETS}

clean:
	rm ${TARGETS}

lib/libexpat.a: expat-2.1.0
	cd $< && ../configure-expat && make install	

lib/libfreetype.a: freetype-2.4.8
	cd $< && ../configure-freetype && make install

lib/libz.a: zlib-1.2.8
	cd $< && ../configure-zlib && make install

expat-2.1.0:
	tar xf $@.tar.gz

freetype-2.4.8:
	tar xf $@.tar.bz2

zlib-1.2.8:
	tar xf $@.tar.gz


en.img:
	dd if=/dev/zero of=$@ bs=1M count=4
	mformat -i $@
	mmd -i $@ Book
	mmd -i $@ Font
	mcopy -i $@ en/*.epub ::Book
	mcopy -i $@ en/*.ttf ::Font
	mdir -i $@

clean:
	- rm en.img
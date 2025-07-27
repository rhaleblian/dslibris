en.img:
	dd if=/dev/zero of=$@ bs=1M count=64
	mformat -i $@
	mmd -i $@ Book
	mmd -i $@ Font
	mcopy -i $@ en/*.epub ::Book
	mcopy -i $@ en/*.ttf ::Font
	mcopy -i $@ melonds.txt ::melonds.txt
	mdir -i $@
	mdir -i $@ book
	mdir -i $@ font

clean:
	- rm en.img
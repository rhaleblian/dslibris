en.img:
	dd if=/dev/zero of=$@ bs=1M count=256
	mformat -i $@
	mmd -i $@ book
	mmd -i $@ font
	mcopy -i $@ en/Book/*.epub ::Book
	mcopy -i $@ en/Font/*.ttf ::Font
	mcopy -i $@ melonds.txt ::melonds.txt

	mdir -i $@
	mdir -i $@ book
	mdir -i $@ font

clean:
	- rm en.img

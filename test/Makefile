en.img:
	dd if=/dev/zero of=$@ bs=1M count=256
	mformat -i $@
	mmd -i $@ book
	mmd -i $@ font
	mcopy -i $@ en/book/*.epub ::book
	mcopy -i $@ en/font/*.ttf ::font
	mcopy -i $@ melonds.txt ::melonds.txt

	mdir -i $@
	mdir -i $@ book
	mdir -i $@ font

clean:
	- rm en.img

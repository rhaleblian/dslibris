.PHONY = cflash
cflash:
	install -d cflash cflash/book
	install dslibris.xml cflash/dslibris.xml
	install font cflash/font
	install book/ds4096.epub cflash/book/ds4096.epub	

%.s %.h : %.png %.grit
	$(DEVKITARM)/bin/grit $< -fts -o$*

splash.s:

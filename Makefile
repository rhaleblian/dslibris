#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
	$(error "Set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/ds_rules

export TARGET		:=	dslibris
export TOPDIR		:=	$(CURDIR)

#-------------------------------------------------------------------------------
# path to tools
#-------------------------------------------------------------------------------
export PATH			:=	$(DEVKITARM)/bin:$(PATH)

# MEDIATYPE should match a DLDI driver name.
MEDIATYPE			:=	mpcf

#-------------------------------------------------------------------------------
# Device emulation.
#-------------------------------------------------------------------------------
# Location of desmume.
EMULATOR			:=	desmume-cli
# Compact flash image file for emulation.
CFLASH_IMAGE		:=	cflash.img
# Path to access mounted emulation image.
CFLASH_MOUNTPOINT	:=	./cflash

.PHONY: $(TARGET).arm7 $(TARGET).arm9

#-------------------------------------------------------------------------------
# main targets
#-------------------------------------------------------------------------------
all: $(TARGET).ds.gba
$(TARGET).ds.gba	: $(TARGET).nds

#-------------------------------------------------------------------------------
$(TARGET).nds		: $(TARGET).arm7 $(TARGET).arm9
	ndstool -b data/icon.bmp \
	"dslibris;an ebook reader;for the Nintendo DS" \
	-c $(TARGET).nds -7 arm7/$(TARGET).arm7 -9 arm9/$(TARGET).arm9

#-------------------------------------------------------------------------------
$(TARGET).arm7		: arm7/$(TARGET).elf
$(TARGET).arm9		: arm9/$(TARGET).elf

#-------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	$(MAKE) -C arm7

#-------------------------------------------------------------------------------
arm9/$(TARGET).elf:
	$(MAKE) -C arm9

#-------------------------------------------------------------------------------
clean:
	$(MAKE) -C arm9 clean
	$(MAKE) -C arm7 clean
	rm -f $(TARGET).ds.gba $(TARGET).nds $(TARGET).$(MEDIATYPE).nds

#-------------------------------------------------------------------------------
# Platform specifics.
#-------------------------------------------------------------------------------
include Makefile.$(shell uname)

# Run under emulator with an image file.
# Use a post-0.9.6 desmume that DLDI-autopatches.
test: $(TARGET).nds
	$(EMULATOR) --cflash-image=$(CFLASH_IMAGE) $(TARGET).nds

# Debug target with insight and desmume under linux
debug: $(TARGET).nds
	arm-eabi-insight arm9/$(TARGET).arm9.elf &
	$(EMULATOR) --cflash-image=$(CFLASH_IMAGE) --arm9gdb=20000 $(TARGET).nds &

debug7: $(TARGET).nds
	arm-eabi-insight arm7/$(TARGET).arm7.elf &
	$(EMULATOR) --cflash-image=$(CFLASH_IMAGE) --arm7gdb=20001 $(TARGET).nds &

gdb: $(TARGET).nds
	$(EMULATOR) --cflash-image=$(CFLASH_IMAGE) --arm9gdb=20000 $(TARGET).nds &
	sleep 4
	arm-eabi-gdb -x data/gdb.commands arm9/$(TARGET).arm9.elf

# make DLDI patched target
$(TARGET).$(MEDIATYPE).nds: $(TARGET).nds
	cp dslibris.nds dslibris.$(MEDIATYPE).nds
	dlditool data/dldi/$(MEDIATYPE).dldi dslibris.$(MEDIATYPE).nds

dldi: $(TARGET).$(MEDIATYPE).nds

# copy target to mounted microSD at $(MEDIAROOT)
install: $(TARGET).nds
	cp $(TARGET).nds $(MEDIAROOT)
	sync

# installation including DLDI patching.
install-dldi: $(TARGET).$(MEDIATYPE).nds
	cp $(TARGET).$(MEDIATYPE).nds $(MEDIAROOT)
	sync

doc: Doxyfile
	doxygen

browse: doc
	firefox doc/html/index.html

# make an archive to release on Sourceforge
dist/$(TARGET).zip: $(TARGET).nds INSTALL.txt
	- mkdir dist
	- rm -r dist/*
	cp INSTALL.txt $(TARGET).nds data/$(TARGET).xml dist
	- mkdir dist/font
	cp data/font/*.ttf dist/font
	- mkdir dist/book	
	cp data/book/dslibris.xht dist/book
	(cd dist; zip -r $(TARGET).zip *)

dist: dist/$(TARGET).zip


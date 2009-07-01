#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/ds_rules

export TARGET		:=	dslibris
export TOPDIR		:=	$(CURDIR)

export MEDIAROOT	:=	/media/SANDISK/
export MEDIATYPE	:=	cyclods
export EMULATOR		:=	desmume-cli

#-------------------------------------------------------------------------------
# path to tools - this can be deleted if you set the path in windows
#-------------------------------------------------------------------------------
export PATH		:=	$(DEVKITARM)/bin:$(PATH)

.PHONY: $(TARGET).arm7 $(TARGET).arm9

#-------------------------------------------------------------------------------
# main targets
#-------------------------------------------------------------------------------
all: $(TARGET).ds.gba
$(TARGET).ds.gba	: $(TARGET).nds

#-------------------------------------------------------------------------------
$(TARGET).nds		: $(TARGET).arm7 $(TARGET).arm9
	ndstool -b data/icon.bmp "dslibris;an ebook reader;for the Nintendo DS" -c $(TARGET).nds -7 arm7/$(TARGET).arm7 -9 arm9/$(TARGET).arm9

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

# run under emulator with an image file.
test: $(TARGET).nds
	$(EMULATOR) --cflash=media.img dslibris.nds

#create a cflash image for use with desmume.
image:
	dd if=/dev/zero of=media.img bs=1048576 count=8
	/sbin/mkfs.vfat media.img
		
# debug target with insight and desmume under linux
debug: $(TARGET).nds
	arm-eabi-insight arm9/dslibris.arm9.elf &
	$(EMULATOR) --cflash=media.img --arm9gdb=20000 $(TARGET).nds &

debug7: $(TARGET).nds
	arm-eabi-insight arm7/dslibris.arm7.elf &
	$(EMULATOR) --cflash=media.img --arm7gdb=20001 $(TARGET).nds &
	
gdb: $(TARGET).nds
	desmume-cli --arm9gdb=20000 --arm7gdb=20001 $(TARGET).nds &
	sleep 4
	arm-eabi-gdb -x gdb.commands arm9/dslibris.arm9.elf

# make DLDI patched target
$(TARGET).$(MEDIATYPE).nds: $(TARGET).nds
	cp dslibris.nds dslibris.$(MEDIATYPE).nds
	dlditool $(MEDIATYPE).dldi dslibris.$(MEDIATYPE).nds

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

# make an archive to release on Sourceforge
dist/$(TARGET).zip: $(TARGET).nds INSTALL.txt
	- mkdir dist
	- rm -r dist/*
	cp INSTALL.txt $(TARGET).nds data/$(TARGET).xml dist
	- mkdir dist/font
	cp data/font/*.ttf dist/font
	- mkdir dist/book	
	cp data/book/dslibris.xht dist/book
	(cd dist; zip -r dslibris.zip *)

dist: dist/$(TARGET).zip

# transfer a release zip for posting.
upload: dist
	cadaver https://frs.sourceforge.net/r/ra/rayh23/uploads

mount:
	- mkdir media
	chmod a+w media
	sudo mount -t vfat -o loop -o rw,uid=$(USER),gid=$(USER) media.img media

umount:
	sync
	- sudo umount media
	- rmdir media


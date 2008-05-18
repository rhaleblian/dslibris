#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

include $(DEVKITARM)/ds_rules

export TARGET		:=	dslibris
export TOPDIR		:=	$(CURDIR)

# symlink the mountpoint for your sd card to 'install'
export MEDIAROOT	:=	install
export MEDIATYPE	:=	M3DSREAL

#-------------------------------------------------------------------------------
# path to tools - this can be deleted if you set the path in windows
#-------------------------------------------------------------------------------
export PATH		:=	$(DEVKITARM)/bin:$(PATH)

.PHONY: $(TARGET).arm7 $(TARGET).arm9

#-------------------------------------------------------------------------------
# main targets
#-------------------------------------------------------------------------------
all			: $(TARGET).ds.gba
$(TARGET).ds.gba	: $(TARGET).nds

#-------------------------------------------------------------------------------
$(TARGET).nds		: $(TARGET).arm7 $(TARGET).arm9
	ndstool -b data/logo.bmp "dslibris;an ebook reader;for the Nintendo DS" -c $(TARGET).nds -7 arm7/$(TARGET).arm7 -9 arm9/$(TARGET).arm9

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

# run target with desmume
test: $(TARGET).nds
	- mkdir -p /tmp/$(TARGET)
	cp $(TARGET).nds /tmp/$(TARGET)
	- mkdir -p /tmp/$(TARGET)/font
	cp data/font/dslibris.ttf /tmp/$(TARGET)/font
	cp data/font/dslibrisb.ttf /tmp/$(TARGET)/font
	cp data/font/dslibrisi.ttf /tmp/$(TARGET)/font
	(cd /tmp/$(TARGET); desmume-cli $(TARGET).nds)

# uses a vfat file under Fedora.
test-vfat: $(TARGET).nds
	desmume-cli --cflash=media.img dslibris.nds

vfat-image:
	dd if=/dev/zero of=media.img bs=1048576 count=32
	/sbin/mkfs.msdos -F16 media.img
		
# debug target with insight and desmume under linux
debug: $(TARGET).nds
	arm-eabi-insight arm9/dslibris.arm9.elf &
	desmume-cli --cflash=media.img --arm9gdb=20000 $(TARGET).nds &

gdb: $(TARGET).nds
	desmume-cli --arm9gdb=20000 --arm7gdb=20001 $(TARGET).nds &
	sleep 4
	$(DEVKITARM)/bin/arm-eabi-gdb -x gdb.commands arm9/dslibris.arm9.elf

# make DLDI patched target
$(TARGET).$(MEDIATYPE).nds: $(TARGET).nds
	cp dslibris.nds dslibris.$(MEDIATYPE).nds
	dlditool $(MEDIATYPE).dldi dslibris.$(MEDIATYPE).nds

# copy target to mounted microSD symlinked to $(MEDIAROOT)
install: $(TARGET).nds
	cp $(TARGET).nds $(MEDIAROOT)
	sync

# installation including DLDI patching.
install-dldi: $(TARGET).$(MEDIATYPE).nds
	cp $(TARGET).$(MEDIATYPE).nds $(MEDIAROOT)
	sync

# make an archive to release on Sourceforge
dist/$(TARGET).zip: $(TARGET).nds INSTALL.txt
	- mkdir dist
	- rm dist/*
	cp INSTALL.txt $(TARGET).nds data/$(TARGET).xht data/$(TARGET).ttf data/$(TARGET).xml dist
	(cd dist; zip -r dslibris.zip *)

dist: dist/$(TARGET).zip

# transfer a release zip for posting.
upload: dist
	ftp upload.sourceforge.net

mount:
	chmod u+w media
	sudo mount -t vfat -o loop -o uid=rhaleblian media.img media

umount:
	sync
	- sudo umount media
	chmod u-w media


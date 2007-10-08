#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

include $(DEVKITARM)/ds_rules

export TARGET		:=	dslibris
export TOPDIR		:=	$(CURDIR)
export MEDIA		:=  R4tf

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
	rm -f $(TARGET).ds.gba $(TARGET).nds $(TARGET).$(MEDIA).nds

# run target in desmume
test: $(TARGET).nds
	desmume $(TARGET).nds

# debug target with insight and desmume in linux
debug: $(TARGET).nds
	arm-eabi-insight arm9/dslibris.arm9.elf &
	desmume --arm9gdb=20000 $(TARGET).nds

# make R4DS DLDI patched target
$(TARGET).$(MEDIA).nds: $(TARGET).nds
	cp dslibris.nds dslibris.$(MEDIA).nds
	dlditool $(MEDIA).dldi dslibris.$(MEDIA).nds

# secure copy target to another machine
scp: $(TARGET).$(MEDIA).nds
	scp $(TARGET).$(MEDIA).nds eris:

# copy target over network to microSD mounted under windows
smb: $(TARGET).$(MEDIA).nds
	 smbclient \\\\asherah\\e fnord... -c "cd .; put dslibris.$(MEDIA).nds"

# copy target to microSD mounted under Linux
usb: $(TARGET).$(MEDIA).nds
	cp $(TARGET).$(MEDIA).nds /media/Kingston
	sync

# make an archive to release on Sourceforge
dist: $(TARGET).nds
	zip dslibris.zip INSTALL.txt $(TARGET).nds $(TARGET).ttf $(TARGET).xht


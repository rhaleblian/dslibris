#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

include $(DEVKITARM)/ds_rules

export TARGET		:=	$(shell basename $(CURDIR))
export TOPDIR		:=	$(CURDIR)


#---------------------------------------------------------------------------------
# path to tools - this can be deleted if you set the path in windows
#---------------------------------------------------------------------------------
export PATH		:=	$(DEVKITARM)/bin:$(PATH)

.PHONY: $(TARGET).arm7 $(TARGET).arm9

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: $(TARGET).ds.gba

$(TARGET).ds.gba	: $(TARGET).nds

#---------------------------------------------------------------------------------
$(TARGET).nds	:	$(TARGET).arm7 $(TARGET).arm9
	ndstool -b data/logo.bmp "dslibris;an ebook reader;for the Nintendo DS" -c $(TARGET).nds -9 arm9/$(TARGET).arm9

#---------------------------------------------------------------------------------
$(TARGET).arm7	: arm7/$(TARGET).elf
$(TARGET).arm9	: arm9/$(TARGET).elf

#---------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	$(MAKE) -C arm7

#---------------------------------------------------------------------------------
arm9/$(TARGET).elf:
	$(MAKE) -C arm9

#---------------------------------------------------------------------------------
clean:
	$(MAKE) -C arm9 clean
	$(MAKE) -C arm7 clean
	rm -f $(TARGET).ds.gba $(TARGET).nds $(TARGET).r4.nds

run: $(TARGET).nds
	desmume $(TARGET).nds

debug: $(TARGET).nds
	desmume --arm9gdb=20000 $(TARGET).nds
#	arm-eabi-insight &

$(TARGET).r4.nds: $(TARGET).nds
	cp dslibris.nds dslibris.r4.nds
	dlditool R4tf.dldi dslibris.r4.nds

smb: $(TARGET).r4.nds
	 smbclient \\\\asherah\\e fnord... -c 'cd .; put dslibris.r4.nds'

usb: $(TARGET).r4.nds
	cp $(TARGET).r4.nds /media/Kingston
	sync

zip: $(TARGET).nds
	zip dslibris.zip $(TARGET).nds $(TARGET).ttf *.xht

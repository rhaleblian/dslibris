#
#
# dslibris - an ebook reader for the Nintendo DS.
#
#  Copyright (C) 2007-2014 Ray Haleblian (ray23@sourceforge.net)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
#

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

.PHONY: $(TARGET).arm7 $(TARGET).arm9 \
	browse debug debug7 debug9 dist dldi doc install install-dldi gdb

#-------------------------------------------------------------------------------
# main targets
#-------------------------------------------------------------------------------
all: $(TARGET).ds.gba
$(TARGET).ds.gba	: $(TARGET).nds

#-------------------------------------------------------------------------------
$(TARGET).nds		: $(TARGET).arm7 $(TARGET).arm9
	ndstool -b data/icon.bmp \
	"dslibris;an ebook reader;for Nintendo DS" \
 	-g LBRS YO 'dslibris' 2 \
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
# Use desmume 0.9.7+ with DLDI-autopatch.
test: $(TARGET).nds
	$(EMULATOR) --preload-rom --cflash-image $(CFLASH_IMAGE) $(TARGET).nds

# Debug under emulation and arm-eabi-insight 6.8.1. Linux only.
# First, build with 'DEBUG=1 make'
debug9: $(TARGET).nds
	arm-eabi-insight arm9/$(TARGET).arm9.elf &
	$(EMULATOR) --preload-rom --cflash-image $(CFLASH_IMAGE) --arm9gdb=20000 $(TARGET).nds &

debug7: $(TARGET).nds
	arm-eabi-insight arm7/$(TARGET).arm7.elf &
	$(EMULATOR) --preload-rom --cflash-image $(CFLASH_IMAGE) --arm7gdb=20001 $(TARGET).nds &

debug:	debug9

# Debug under emulation and arm-eabi-gdb.
gdb: $(TARGET).nds
	$(EMULATOR) --preload-rom --cflash-image $(CFLASH_IMAGE) \
	 --arm9gdb=20000 $(TARGET).nds &
	sleep 4
	arm-eabi-gdb -x data/gdb.commands arm9/$(TARGET).arm9.elf

# Make DLDI patched target.
$(TARGET).$(MEDIATYPE).nds: $(TARGET).nds
	cp dslibris.nds dslibris.$(MEDIATYPE).nds
	dlditool data/dldi/$(MEDIATYPE).dldi dslibris.$(MEDIATYPE).nds

dldi: $(TARGET).$(MEDIATYPE).nds

# Copy target to mounted microSD at $(MEDIA_MOUNTPOINT)
install: $(TARGET).nds
	cp $(TARGET).nds $(MEDIA_MOUNTPOINT)
	sync

# Installation as above, including DLDI patching.
install-dldi: $(TARGET).$(MEDIATYPE).nds
	cp $(TARGET).$(MEDIATYPE).nds $(MEDIA_MOUNTPOINT)/$(TARGET).nds
	sync

doc: Doxyfile
	doxygen

browse: doc
	firefox doc/html/index.html

# Make an archive to release on Sourceforge.
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


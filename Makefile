#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(shell basename $(CURDIR))
BUILD		:=	build
SOURCES		:=	gfx source data  
INCLUDES	:=	include build

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-mthumb -mthumb-interwork

CFLAGS	:=	-g -Wall -O2 \
			-I$(DEVKITPRO)/portlibs/nds/include/freetype2 \
 			-march=armv5te -mtune=arm946e-s -fomit-frame-pointer \
			-ffast-math \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM9
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:= -lfat -lnds9 -lfreetype -lexpat -lz -lbz2 -lpng

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBNDS) $(DEVKITPRO)/portlibs/nds
 
#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
 
export OUTPUT	:=	$(CURDIR)/$(TARGET)
 
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.bin)))
 
#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(BINFILES:.bin=.o) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
 
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)
 
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
 
.PHONY: $(BUILD) clean
 
#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
 
#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).ds.gba 
 
 
#---------------------------------------------------------------------------------
else
 
DEPENDS	:=	$(OFILES:.o=.d)
 
#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).nds	: 	$(OUTPUT).elf
$(OUTPUT).elf	:	$(OFILES)
 
#---------------------------------------------------------------------------------
%.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)

#---------------------------------------------------------------------------------
# This rule creates assembly source files using grit
# grit takes an image file and a .grit describing how the file is to be processed
# add additional rules like this for each image extension
# you use in the graphics folders
#---------------------------------------------------------------------------------
%.s %.h: %.png %.grit
#---------------------------------------------------------------------------------
	grit $< -fts -o$*
 
-include $(DEPENDS)
 
#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------


#----- Local rules beyond this point -- "Abandon hope...", etc --------------#

CFLASHIMAGE = dslibris.img
MOUNTPOINT = mnt
TOOLSBIN = $(DEVKITPRO)/tools/bin

$(MOUNTPOINT):
	- mkdir $(MOUNTPOINT)

# Create a FAT image file for DeSMuME's --cflash-image option.
$(CFLASHIMAGE):
	dd if=/dev/zero of=$@ bs=1M count=16
	mkfs.fat -n DSLIBRIS $@
	make mount
	install --target-directory $(MOUNTPOINT) etc/root/*
	make umount

.PHONY: test log mount umount patch

header:
	$(TOOLSBIN)/ndstool -h 0x400 -g LBRS YO dslibris $(shell cat version.txt) $(TARGET).nds

patch:
	$(TOOLSBIN)/dlditool etc/mpcf.dldi dslibris.nds

test: $(BUILD) $(CFLASHIMAGE) patch
	desmume --cflash-image $(CFLASHIMAGE) $(TARGET).nds

gdb:
	desmume --cflash-image $(CFLASHIMAGE) --arm9gdb=9000 $(TARGET).nds &
	sleep 2
	$(DEVKITARM)/bin/arm-none-eabi-gdb --init-command=etc/gdb-init-commands $(TARGET).elf
	# Stop both GDB and DeSMuME when done.

mount: $(CFLASHIMAGE) $(MOUNTPOINT)
	- sudo mount -o loop $(CFLASHIMAGE) $(MOUNTPOINT)
	ls $(MOUNTPOINT)

umount:
	sudo umount $(MOUNTPOINT)
	rmdir $(MOUNTPOINT)

# Fetch the log from inside the cflash image.
log: mount
	- sudo mount -o loop $(CFLASHIMAGE) $(MOUNTPOINT)
	sudo mv $(MOUNTPOINT)/$(TARGET).log .
	make umount

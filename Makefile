#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= /opt/devkitpro/devkitARM

VERSION ?= $(cat include/version.h)

GAME_TITLE := dslibris
GAME_SUBTITLE1 := An EPUB ebook reader [$(VERSION)]
GAME_SUBTITLE2 := https://github.com/rhaleblian/dslibris
GAME_ICON := $(PWD)/gfx/icon.bmp

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

CFLAGS	:=	-Wall -O2 \
		-march=armv5te -mtune=arm946e-s -fomit-frame-pointer \
		-ffast-math \
		$(ARCH)

# symbols, for maybe getting GDB to work again
# CFLAGS	+=	-g

# elide code that borks melonDS 
# CFLAGS	+=	-DMELONDS

CFLAGS  +=	-I$(PWD)/portlibs/nds/include/freetype2
CFLAGS	+=	$(INCLUDE) -DARM9
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:= -lfat -lnds9 -lexpat -lz -lbz2 -lpng -lfreetype

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBNDS) $(PWD)/portlibs/nds $(DEVKITPRO)/portlibs/nds

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

.PHONY: abandonhope doc check distcheck distclean

check:
	true

doc:
	doxygen

distcheck:
	true

distclean: clean
	- rm dslibris-*.zip
	- rm test/*.img

abandonhope: distclean

#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

GAME_TITLE := dslibris
GAME_SUBTITLE1 := An EPUB reader for Nintendo DS
GAME_SUBTITLE2 := 1.6 Yoyodyne Research
GAME_ICON := $(PWD)/data/icon.bmp

DESMUME := desmume-cli
# DESMUME := /Applications/DeSmuME.app/Contents/MacOS/DeSmuME
ARM9GDBPORT := 9000
PORTLIBS := $(DEVKITPRO)/portlibs/nds
INCLUDE_FREETYPE := -I$(PORTLIBS)/include/freetype2

include $(DEVKITARM)/ds_rules

#-------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#-------------------------------------------------------------------------------
TARGET		:=	$(shell basename $(CURDIR))
BUILD		:=	build
SOURCES		:=	source
DATA		:=  
INCLUDES	:=	include
GRAPHICS	:=	data

#-------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv5te -mtune=arm946e-s -mthumb

CFLAGS	:=	-g -Wall -O2 -ffunction-sections -fdata-sections\
		$(ARCH)

CFLAGS	+=	$(INCLUDE) $(INCLUDE_FREETYPE) -DARM9
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#-------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#-------------------------------------------------------------------------------
LIBS	:= -lfat -lnds9 -lfreetype -lexpat -lz -lbz2 -lpng

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBNDS) $(PORTLIBS)

#-------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#-------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#-------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
					$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))
PNGFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.png)))

#-------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#-------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#-------------------------------------------------------------------------------
	export LD	:=	$(CC)
#-------------------------------------------------------------------------------
else
#-------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(PNGFILES:.png=.o) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

#-------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#-------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).ds.gba 


#-------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#-------------------------------------------------------------------------------
# main targets
#-------------------------------------------------------------------------------
$(OUTPUT).nds	: 	$(OUTPUT).elf
$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)


#---------------------------------------------------------------------------------
%.s %.h	: %.png %.grit
#---------------------------------------------------------------------------------
	grit $< -fts -o$*

-include $(DEPENDS)

#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------


#----- Local rules beyond this point -- "Abandon hope...", etc --------------#

DESMUME := "$(HOME)/Applications/DeSmuME (Debug).app/Contents/MacOS/DeSmuME (debug)"
# DESMUME := ../desmume/desmume/src/frontend/posix/build/cli/desmume-cli
# DESMUME := /Users/ray/Library/Developer/Xcode/DerivedData/DeSmuME_\(Latest\)-ewlwovwrwltvgmanisirmngyqxuk/Build/Products/Release/DeSmuME.app/Contents/MacOS/DeSmuME

.PHONY: debug doc markdown run upload

run: $(OUTPUT).nds
	$(DESMUME) --cflash-path $(PWD)/cflash $(OUTPUT).nds

debug: $(OUTPUT).nds
	$(DESMUME) --arm9gdb=9000 --cflash-path $(PWD)/cflash $(OUTPUT).nds

release.zip: $(OUTPUT).nds
	dlditool etc/dldi/r4tf_v2.dldi $(OUTPUT).nds
	zip release.zip dslibris.nds
	(cd etc/filesystem/en; zip -r -u ../../../release.zip .)

doc:
	doxygen
	echo "Documentation is located at ./doc/html ."

markdown: doc
	node ../moxygen/bin/moxygen.js -h doc/xml

cflash:
	mkdir cflash
	cp -r etc/filesystem/en/Book cflash/
	cp -r etc/filesystem/en/Font cflash/

cflash.img: cflash
	#dd if=/dev/zero of=cflash.img bs=1k count=512
	/usr/sbin/mkfs.fat -C cflash.img 2048
	mcopy -i cflash.img -s cflash/* ::/
	@echo "cflash.img created with Book and Font directories."
	@echo "You can now use cflash.img with DeSmuME."
	@echo "Run 'make run' to test it."
	@echo "Run 'make upload' to upload it to your DS."
	@echo "Run 'make release.zip' to create a release zip with the .nds file."

run: cflash
	$(DESMUME) --cflash-image cflash.img $(OUTPUT).nds

debug: cflash
	$(DESMUME) --arm9gdb=$(ARM9GDBPORT) --cflash-path ./cflash $(OUTPUT).nds

upload:
	ftp -u ftp://ray:ray@${DS_HOST}:5000/ dslibris.nds

release.zip:
	dlditool etc/dldi/r4tf_v2.dldi $(OUTPUT).nds
	zip release.zip $(OUTPUT).nds
	(cd etc/filesystem/en; zip -r -u ../../../release.zip .)

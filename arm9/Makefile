#-----------------------------------------------------------------------
.SUFFIXES:
#-----------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

include $(DEVKITARM)/ds_rules

#-----------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#-----------------------------------------------------------------------
#TARGET		:=	$(shell basename $(CURDIR))
TARGET		:=	dslibris
BUILD		:=	build
SOURCES		:=	.
INCLUDES	:=	../include build ../../local/include/freetype2

#-----------------------------------------------------------------------
# options for code generation
#-----------------------------------------------------------------------
ARCH	:=	-mthumb -mthumb-interwork

# note: arm9tdmi isn't the correct CPU arch, but anything newer and LD
# *insists* it has a FPU or VFP, and it won't take no for an answer!
CFLAGS	:=	-g -Wall -O0 \
		-mcpu=arm968e-s -mtune=arm968e-s \
#		-mcpu=arm7tdmi -mtune=arm9tdmi \
		-fomit-frame-pointer \
		-ffast-math \
		$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM9
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -mno-fpu -Wl,-Map,$(notdir $*.map)

#-----------------------------------------------------------------------
# any extra libraries we wish to link with the project
#-----------------------------------------------------------------------
LIBS	:= -ldswifi9 -lfat -lnds9 -lfreetype -lexpat

#-----------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#-----------------------------------------------------------------------
LIBDIRS	:=	 $(LIBNDS) $(HOME)/nds/local

#-----------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#-----------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#-----------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.bin)))

#-----------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#-----------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#-----------------------------------------------------------------------
	export LD	:=	$(CC)
#-----------------------------------------------------------------------
else
#-----------------------------------------------------------------------
	export LD	:=	$(CXX)
#-----------------------------------------------------------------------
endif
#-----------------------------------------------------------------------

export OFILES	:=	$(BINFILES:.bin=.o) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/lib) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) export dldi test install

all: $(BUILD)
#-----------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#-----------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD)
	@rm -fr $(TARGET).arm9.elf $(TARGET).arm9

#-----------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#-----------------------------------------------------------------------
# main targets
#-----------------------------------------------------------------------
$(OUTPUT).arm9	:	$(OUTPUT).arm9.elf
$(OUTPUT).arm9.elf	:	$(OFILES)

#-----------------------------------------------------------------------
%.o	:	%.bin
#-----------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)


-include $(DEPENDS)

#-----------------------------------------------------------------------------
endif
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------
.SUFFIXES:
#-----------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/ds_rules

#-----------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory for object files & intermediate files
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#-----------------------------------------------------------------------
#TARGET		:=	$(shell basename $(CURDIR))
TARGET		:=	dslibris
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	../include build

#-----------------------------------------------------------------------
# options for code generation
#-----------------------------------------------------------------------
ARCH	:=	-mthumb -mthumb-interwork \
		-mcpu=arm7tdmi -mtune=arm7tdmi

CFLAGS	:=	-Wformat=2 -Wall -Winline \
		-fomit-frame-pointer -ffast-math \
		$(ARCH) $(INCLUDE) -DARM7
ifeq ($(DEBUG),1)
CFLAGS +=	-g
else
CFLAGS +=	-O2
endif

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm7.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#-----------------------------------------------------------------------
# any extra libraries we wish to link with the project
#-----------------------------------------------------------------------
LIBS	:=	-lnds7

#-----------------------------------------------------------------------
# list of directories containing libraries, this must be the top level
# containing include and lib
#-----------------------------------------------------------------------
LIBDIRS	:=	 $(LIBNDS)

#-----------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add
# additional rules for different file extensions
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

.PHONY: $(BUILD)

all: $(BUILD)
#-----------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#-----------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD)
	@rm -fr $(TARGET).arm7.elf $(TARGET).arm7 

#-----------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#-----------------------------------------------------------------------
# main targets
#-----------------------------------------------------------------------
$(OUTPUT).arm7	:	$(OUTPUT).arm7.elf
$(OUTPUT).arm7.elf	:	$(OFILES)

#-----------------------------------------------------------------------
%.o	:	%.bin
#-----------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)


-include $(DEPENDS)

#-----------------------------------------------------------------------------
endif
#-----------------------------------------------------------------------------

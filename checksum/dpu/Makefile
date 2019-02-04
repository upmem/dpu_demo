#Compiler and Linker
CC          := clang --target=dpu-upmem-dpurte
#Configurator
KCONF       := dpukconfig
DPUCC       := dpucc

#The Target Binary Program
TARGET      := dpu_app

#The Directories, Source, Includes, Objects, Binary
SRCDIR      := src
INCDIR      := inc
BUILDDIR    := ../obj/dpu
TARGETDIR   := ../bin/dpu
SRCEXT      := c
ASMEXT      := s
OBJEXT      := o
CFGEXT      := json
BINEXT      := bin

#Flags, Libraries and Includes
DBG         := -g
CFLAGS      := -Wall -O2 $(DBG)
LIB         := 
INC         := -I$(INCDIR)

CFG_NAME    := configuration
CFG_SCRIPT  := build_configuration.script

SOURCES     := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
ASM_SOURCES := $(shell find $(SRCDIR) -type f -name *.$(ASMEXT))
C_OBJECTS   := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))
ASM_OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(ASM_SOURCES:.$(ASMEXT)=.$(OBJEXT)))

BOOTSTRAP   := __bootstrap

EXEC        := $(TARGETDIR)/$(TARGET).$(BINEXT)

.PHONY: all remake clean cleaner
.DEFAULT_GOAL := all

all: directories $(EXEC)

remake: cleaner all

#Make the Directories
directories:
	@mkdir -p $(TARGETDIR)
	@mkdir -p $(BUILDDIR)

#Link
$(EXEC): $(BUILDDIR)/$(BOOTSTRAP).$(OBJEXT) $(C_OBJECTS) $(ASM_OBJECTS)
	$(CC) -o $@ $^ $(LIB) $(DBG)

#Assemble bootstrap
$(BUILDDIR)/$(BOOTSTRAP).$(OBJEXT): $(BUILDDIR)/$(BOOTSTRAP).$(ASMEXT)
	$(CC) -c -o $@ $(CFLAGS) $^ $(INC)

#Compile
$(ASM_OBJECTS): $(BUILDDIR)/%.$(OBJEXT) : $(SRCDIR)/%.$(ASMEXT)
	$(CC) -c -o $@ $(CFLAGS) $^ $(INC)

$(C_OBJECTS): $(BUILDDIR)/%.$(OBJEXT) : $(SRCDIR)/%.$(SRCEXT)
	$(CC) -c -o $@ $(CFLAGS) $^ $(INC)

#Generate Bootstrap
$(BUILDDIR)/$(BOOTSTRAP).$(ASMEXT): $(BUILDDIR)/$(CFG_NAME).$(CFGEXT)
	$(DPUCC) -xc $^ -o $(BUILDDIR)

#Build configuration
$(BUILDDIR)/$(CFG_NAME).$(CFGEXT): $(CFG_SCRIPT)
	$(KCONF) -s $^ -v DIRECTORY=$(BUILDDIR) -v NAME=$(CFG_NAME)

clean:
	@$(RM) -rf $(BUILDDIR)

cleaner: clean
	@$(RM) -rf $(TARGETDIR)

#Compiler and Linker
CC          := dpucc
#Configurator
KCONF       := dpukconfig

#The Target Binary Program
TARGET      := dpu_app

#The Directories, Source, Includes, Objects, Binary
SRCDIR      := src
INCDIR      := inc
BUILDDIR    := ../obj/dpu
TARGETDIR   := ../bin/dpu
SRCEXT      := c
ASMEXT      := s
OBJEXT      := ipr
CFGEXT      := json
BINEXT      := bin

#Flags, Libraries and Includes
DBG         := -g
CFLAGS      := -Wall -O1 $(DBG)
LIB         := 
INC         := -I$(INCDIR)

CFG_NAME    := configuration
CFG_SCRIPT  := build_configuration.script

SOURCES     := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
ASM_SOURCES := $(shell find $(SRCDIR) -type f -name *.$(ASMEXT))
C_OBJECTS   := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))
ASM_OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(ASM_SOURCES:.$(ASMEXT)=.$(OBJEXT)))

BOOTSTRAP   := bootstrap

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
	$(CC) -l -o $@ $^ $(LIB) $(DBG)

#Assemble from C sources
$(C_OBJECTS): $(BUILDDIR)/%.$(OBJEXT) : $(BUILDDIR)/%.$(ASMEXT)
	$(CC) -s -f IPR -o $@ $< -I$(BUILDDIR)

#Assemble from ASM sources
$(ASM_OBJECTS): $(BUILDDIR)/%.$(OBJEXT) : $(SRCDIR)/%.$(ASMEXT)
	$(CC) -s -f IPR -o $@ $< -I$(BUILDDIR)

#Assemble bootstrap
$(BUILDDIR)/$(BOOTSTRAP).$(OBJEXT): $(BUILDDIR)/$(BOOTSTRAP).$(ASMEXT)
	$(CC) -s -f IPR -o $@ $< -I$(BUILDDIR)

#Compile
$(BUILDDIR)/%.$(ASMEXT): $(SRCDIR)/%.$(SRCEXT)
	$(CC) -c -o $@ $(CFLAGS) $^ $(INC)

#Generate Bootstrap
$(BUILDDIR)/$(BOOTSTRAP).$(ASMEXT): $(BUILDDIR)/$(CFG_NAME).$(CFGEXT)
	$(CC) -xc $^ -o $(BUILDDIR)

#Build configuration
$(BUILDDIR)/$(CFG_NAME).$(CFGEXT): $(CFG_SCRIPT)
	$(KCONF) -s $^ -v DIRECTORY=$(BUILDDIR) -v NAME=$(CFG_NAME)

clean:
	@$(RM) -rf $(BUILDDIR)

cleaner: clean
	@$(RM) -rf $(TARGETDIR)

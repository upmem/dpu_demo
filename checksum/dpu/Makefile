#Compiler and Linker
CC          := clang --target=dpu-upmem-dpurte

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
BINEXT      := bin

#Flags, Libraries and Includes
DBG         := -g
CFLAGS      := -Wall -O2 $(DBG)
LIB         := 
INC         := -I$(INCDIR)

SOURCES     := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
ASM_SOURCES := $(shell find $(SRCDIR) -type f -name *.$(ASMEXT))
C_OBJECTS   := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))
ASM_OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(ASM_SOURCES:.$(ASMEXT)=.$(OBJEXT)))

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
$(EXEC): $(C_OBJECTS) $(ASM_OBJECTS)
	$(CC) -o $@ $^ $(LIB) $(DBG)

#Compile
$(ASM_OBJECTS): $(BUILDDIR)/%.$(OBJEXT) : $(SRCDIR)/%.$(ASMEXT)
	$(CC) -c -o $@ $(CFLAGS) $^ $(INC)

$(C_OBJECTS): $(BUILDDIR)/%.$(OBJEXT) : $(SRCDIR)/%.$(SRCEXT)
	$(CC) -c -o $@ $(CFLAGS) $^ $(INC)

clean:
	@$(RM) -rf $(BUILDDIR)

cleaner: clean
	@$(RM) -rf $(TARGETDIR)

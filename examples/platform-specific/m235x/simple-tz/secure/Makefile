TZDIR=..
TOP ?= ../../../..
CONTIKI = $(TZDIR)/$(TOP)

PROJECT_SOURCEFILES += main.c puts.c
PROJECT_SOURCEFILES += version.c

CFLAGS:= -Wall -std=c99 -g -O2 -I. -I$(TZDIR)/include

all:

include $(CONTIKI)/Makefile.include

CFLAGS += $(CONTIKI_VERSION)

ELFFILE=secure.$(TARGET)
BINFILE=secure.$(TARGET).bin
MAPFILE=secure.$(TARGET).map
CMSE_LIB=$(BUILD_DIR_BOARD)/cmse_lib.o

CLEAN += $(CMSE_LIB) $(ELFFILE) $(MAPFILE)

CONTIKI_CPU_OBJECTFILES = ${addprefix $(OBJECTDIR)/,${call oname, $(CONTIKI_CPU_SOURCEFILES)}}
CONTIKI_TARGET_SECURE_OBJECTFILES = ${addprefix $(OBJECTDIR)/,${call oname, $(CONTIKI_TARGET_SECURE_SOURCEFILES)}}
LDFLAGS += -Wl,--cmse-implib -Wl,--out-implib=$(CMSE_LIB)
LDFLAGS += -Wl,-Map=$(MAPFILE)

all: $(ELFFILE) Makefile
PROJECT_OBJECTFILES = $(addprefix $(OBJECTDIR)/, $(PROJECT_SOURCEFILES:.c=.o))
$(ELFFILE): $(CONTIKI_CPU_OBJECTFILES) $(CONTIKI_TARGET_SECURE_OBJECTFILES) $(PROJECT_OBJECTFILES)
	$(TRACE_LD)
	$(Q)$(LD) $(LDFLAGS) -o $@ $^
	$(Q)$(OBJCOPY) -O binary --gap-fill=0xff $@ $(BINFILE)

TZDIR=..
CONTIKI = $(TZDIR)/$(TOP)

PROJECT_SOURCEFILES += main.c
PROJECT_SOURCEFILES += version.c

CFLAGS:= -Wall -std=c99 -g -O2 -I. -I$(TZDIR)/include

all:

include $(CONTIKI)/Makefile.include

CLEAN += $(CMSE_LIB) secure.$(TARGET)

CONTIKI_CPU_OBJECTFILES = ${addprefix $(OBJECTDIR)/,${call oname, $(CONTIKI_CPU_SOURCEFILES)}}
CMSE_LIB=$(BUILD_DIR_BOARD)/cmse_lib.o
LDFLAGS += -Wl,--cmse-implib -Wl,--out-implib=$(CMSE_LIB)
LDFLAGS += -Wl,-Map=secure.map

all: secure.$(TARGET)
PROJECT_OBJECTFILES = $(addprefix $(OBJECTDIR)/, $(PROJECT_SOURCEFILES:.c=.o))
secure.$(TARGET): $(CONTIKI_CPU_OBJECTFILES) $(PROJECT_OBJECTFILES)
	$(TRACE_LD)
	$(Q)$(LD) $(LDFLAGS) -o $@ $^

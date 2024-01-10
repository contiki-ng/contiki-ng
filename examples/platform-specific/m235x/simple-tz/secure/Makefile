TZDIR=..
CONTIKI = $(TZDIR)/$(TOP)

PROJECT_SOURCEFILES += version.c
OBJECTS:= $(OBJECTDIR)/$(SOURCES:.c=.o)

CFLAGS:= -Wall -std=c99 -g -O2 -I. -Iinclude
ARFLAGS:=crU

CLEAN += $(LIB)

all: tz_build

include $(CONTIKI)/Makefile.include

LIB=$(BUILD_DIR_BOARD)/libsecure.a

.PHONY: tz_build
tz_build: $(LIB)
$(LIB): $(PROJECT_OBJECTFILES)
	$(AR) $(ARFLAGS) $@ $^
	@ranlib $@

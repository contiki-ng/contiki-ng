CONTIKI_PROJECT = nonsecure
all: postbuild

TZDIR=..
TOP ?= ../../../..
CONTIKI = $(TZDIR)/$(TOP)
NS=1

SECUREDIR=$(TZDIR)/secure
PROJECTDIRS += $(TZDIR)/include

SECURE_BUILD_DIR=$(TZDIR)/secure/build/$(TARGET)/$(BOARD)
LDFLAGS += -L$(SECURE_BUILD_DIR)

TARGET_LIBFILES += $(SECURE_BUILD_DIR)/cmse_lib.o
include $(CONTIKI)/Makefile.include

BINFILE=$(CONTIKI_PROJECT).$(TARGET).bin
MAPFILE=$(CONTIKI_PROJECT).$(TARGET).map
HEXFILE=$(CONTIKI_PROJECT).$(TARGET).hex

LDFLAGS += -Wl,-Map=$(MAPFILE)

CLEAN += $(MAPFILE) $(HEXFILE) $(BINFILE)

postbuild: $(BINFILE) $(HEXFILE)
$(BINFILE): $(CONTIKI_PROJECT).$(TARGET)
	$(Q)$(OBJCOPY) -O binary --gap-fill=0xff $^ $(BINFILE)
$(HEXFILE): $(CONTIKI_PROJECT).$(TARGET)
	$(Q)$(OBJCOPY) -O ihex --change-addresses=-0x10000000 $^ $(HEXFILE)

.PHONY: postbuild

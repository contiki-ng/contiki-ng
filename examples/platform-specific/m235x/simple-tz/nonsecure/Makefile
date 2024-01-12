CONTIKI_PROJECT = nonsecure
all: postbuild

TZDIR=..
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

postbuild: $(CONTIKI_PROJECT).$(TARGET)
	$(Q)$(OBJCOPY) -O binary --gap-fill=0xff $^ $(BINFILE)

.PHONY: postbuild

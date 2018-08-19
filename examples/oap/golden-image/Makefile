CONTIKI_PROJECT = golden-image
all: $(CONTIKI_PROJECT)

MODULES += os/net/app-layer/ota

PLATFORMS_ONLY = srf06-cc26xx cc2538dk openmote-cc2538 zoul

ifeq ($(BOOTLOADER_DEBUG), 1)
  CFLAGS += -DBUILD_WITH_BOOTLOADER_DEBUG=1
endif

CONTIKI = ../../..
-include $(CONTIKI)/Makefile.identify-target

MODULES_REL += arch/platform/$(TARGET)

include $(CONTIKI)/Makefile.include

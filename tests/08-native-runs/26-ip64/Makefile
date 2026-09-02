CONTIKI_PROJECT = test-ip64
all: $(CONTIKI_PROJECT)

TARGET ?= native

CONTIKI = ../../..

WITH_IP64 = 1
MODULES += os/services/unit-test

PROJECT_SOURCEFILES += ip64-test-driver.c

# Same wiring as arch/platform/zoul/orion/Makefile.orion.
CFLAGS += -DUIP_FALLBACK_INTERFACE=ip64_uip_fallback_interface
# Make ip64 the only off-link path, as on a border router.
CFLAGS += -DNATIVE_WITH_IPV6_DEFAULT_ROUTE=0
CFLAGS += -I$(CURDIR)

include $(CONTIKI)/Makefile.include

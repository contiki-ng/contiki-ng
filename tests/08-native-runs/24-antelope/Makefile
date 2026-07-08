CONTIKI_PROJECT = test-antelope
all: $(CONTIKI_PROJECT)

TARGET = native

# Antelope stores its relations in the Coffee file system.
MAKE_CFS = MAKE_CFS_COFFEE

MODULES += os/storage/antelope
MODULES += os/services/unit-test

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include

CONTIKI_PROJECT = compress ecdh ecdsa
all: $(CONTIKI_PROJECT)

CONTIKI = ../../..
include $(CONTIKI)/Makefile.identify-target

ifeq ($(TARGET),native)
# Include software implementation of ECC
MODULES += os/services/ecc
endif

include $(CONTIKI)/Makefile.include

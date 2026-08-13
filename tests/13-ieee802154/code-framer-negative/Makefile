CONTIKI_PROJECT = test-framer-negative
all: $(CONTIKI_PROJECT)

MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/services/unit-test
MODULES += $(CONTIKI_NG_MAC_DIR)/tsch/sixtop

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include

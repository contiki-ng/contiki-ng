CONTIKI_PROJECT      = sixp-node
PROJECT_SOURCEFILES += test-sf.c

PLATFORMS_EXCLUDE = sky z1 native

CONTIKI  = ../../../

MAKE_MAC = MAKE_MAC_TSCH
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_MAC_DIR)/tsch/sixtop

all: $(CONTIKI_PROJECT)
include $(CONTIKI)/Makefile.include

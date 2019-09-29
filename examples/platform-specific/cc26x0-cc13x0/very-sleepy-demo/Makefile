CONTIKI_PROJECT = very-sleepy-demo

PLATFORMS_ONLY = cc26x0-cc13x0

all: $(CONTIKI_PROJECT)

CONTIKI = ../../../..

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

include $(CONTIKI)/Makefile.include

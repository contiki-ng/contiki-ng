CONTIKI_PROJECT = client ifttt-client
all: $(CONTIKI_PROJECT)

BOARD = orouter

WITH_IP64 = 1

PLATFORMS_ONLY = owinos
BOARDS_ONLY = orouter

CONTIKI = ../../../../..

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/http-socket

include $(CONTIKI)/Makefile.include

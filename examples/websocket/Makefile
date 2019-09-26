CONTIKI_PROJECT = http-example websocket-example
all: $(CONTIKI_PROJECT)
CONTIKI=../..

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/http-socket

PLATFORMS_EXCLUDE = sky z1

include $(CONTIKI)/Makefile.include

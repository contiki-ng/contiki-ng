CONTIKI_PROJECT = client ifttt_client
all: $(CONTIKI_PROJECT)

BOARD = orion

MODULES += os/net/app-layer/http-socket

WITH_IP64 = 1

PLATFORMS_ONLY = zoul

CONTIKI = ../../../../..
include $(CONTIKI)/Makefile.include

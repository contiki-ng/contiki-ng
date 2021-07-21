CONTIKI_PROJECT = ip64-router
all: $(CONTIKI_PROJECT)

BOARD = orouter

CONTIKI = ../../../../..

WITH_IP64 = 1

PROJECT_SOURCEFILES += httpd-simple.c

PLATFORMS_ONLY = owinos
BOARDS_ONLY = orouter

include $(CONTIKI)/Makefile.include

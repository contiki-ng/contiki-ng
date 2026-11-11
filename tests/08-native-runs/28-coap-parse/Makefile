CONTIKI_PROJECT = test-coap-parse
all: $(CONTIKI_PROJECT)

TARGET = native

MODULES += os/services/unit-test
MODULES += os/net/app-layer/coap

# Replace the UDP transport with one that captures what the engine sends.
MODULES_SOURCES_EXCLUDES += coap-uip.c
PROJECT_SOURCEFILES += test-transport.c

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include

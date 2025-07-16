CONTIKI_PROJECT = test-bitrev
all: $(CONTIKI_PROJECT)

# Enable bit reversal library for testing
CFLAGS += -DRADIO_CONF_PAYLOAD_BIT_REVERSE=1

TARGET = native

MODULES += os/services/unit-test

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
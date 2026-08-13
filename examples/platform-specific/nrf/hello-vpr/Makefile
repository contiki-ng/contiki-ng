CONTIKI_PROJECT = hello-vpr
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../..
TARGET ?= nrf-vpr
MAKE_NET = MAKE_NET_NULLNET
MAKE_MAC = MAKE_MAC_NULLMAC
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING

# Board-dependent user LED on GPIO port 2 (default P2.00, active low = XIAO
# nRF54L15; pass LED0_PIN=9 LED0_ACTIVE_LOW=0 for the nRF54L15-DK). flpr-host
# passes these through per BOARD.
ifdef LED0_PIN
CFLAGS += -DLED0_PIN=$(LED0_PIN)
endif
ifdef LED0_ACTIVE_LOW
CFLAGS += -DLED0_ACTIVE_LOW=$(LED0_ACTIVE_LOW)
endif

include $(CONTIKI)/Makefile.include

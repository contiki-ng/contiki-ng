CONTIKI_PROJECT = ipc-radio-service
CONTIKI = ../../../../

all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY += nrf
BOARDS_ONLY = nrf5340/dk/network

TARGET ?= nrf
BOARD ?= nrf5340/dk/network

# The IPC MAC driver forwards received frames to the app core via
# shared memory. It replaces NULLMAC to enable interrupt-driven
# frame reception (radio ISR -> radio process -> ipc-mac input).
MAKE_MAC = MAKE_MAC_OTHER
MAKE_NET = MAKE_NET_NULLNET
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING
MODULES_REL += $(CONTIKI_NG_RELOC_CPU_DIR)/nrf/net

include $(CONTIKI)/Makefile.include

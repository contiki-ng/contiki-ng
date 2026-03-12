CONTIKI_PROJECT = secure-world-example

all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY += nrf
BOARDS_ONLY = nrf54l15/xiao

TARGET ?= nrf
BOARD ?= nrf54l15/xiao

PROJECTDIRS += ../../secure-world

# Keep the secure world minimal and let the TrustZone MAC shim own packet delivery.
MAKE_MAC = MAKE_MAC_NULLMAC
MAKE_NET = MAKE_NET_NULLNET

TRUSTZONE_SECURE_BUILD = 1

include ../../../../../../Makefile.include

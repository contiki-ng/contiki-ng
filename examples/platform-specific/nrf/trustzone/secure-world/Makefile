CONTIKI_PROJECT = secure-world-example

all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY += nrf
BOARDS_ONLY = nrf54l15/xiao nrf54l15/dk

TARGET ?= nrf
BOARD ?= nrf54l15/xiao

# Use a minimal network stack to reduce the trusted computing base.
MAKE_MAC = MAKE_MAC_NULLMAC
MAKE_NET = MAKE_NET_NULLNET

TRUSTZONE_SECURE_BUILD = 1

include ../../../../../Makefile.include

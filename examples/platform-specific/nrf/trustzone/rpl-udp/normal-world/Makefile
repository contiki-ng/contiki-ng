CONTIKI_PROJECT ?= udp-client

all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY += nrf
BOARDS_ONLY = nrf54l15/xiao

TARGET ?= nrf
BOARD ?= nrf54l15/xiao

PROJECTDIRS += ../../../../../rpl-udp

MAKE_MAC = MAKE_MAC_CSMA
MAKE_NET = MAKE_NET_IPV6
MAKE_ROUTING = MAKE_ROUTING_RPL_LITE

MODULES += os/services/shell

TZ_MINIMAL_NONSECURE_PLATFORM = 1

TRUSTZONE_SECURE_BUILD = 0
TRUSTZONE_SECURE_WORLD_PATH ?= ../secure-world

include ../../../../../../Makefile.include

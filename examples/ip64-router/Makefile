CONTIKI_PROJECT = ip64-router
all: $(CONTIKI_PROJECT)
CONTIKI=../..

# Currently only supported on Orion, the only platform with 802.15.4 + Ethernet
PLATFORMS_ONLY = zoul
BOARDS_ONLY = orion
BOARD = orion
WITH_IP64 = 1

include $(CONTIKI)/Makefile.include

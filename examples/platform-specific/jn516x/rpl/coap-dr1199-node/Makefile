CONTIKI_PROJECT = dr1199-node
all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY = jn516x

TARGET ?= jn516x
JN516x_WITH_DR1199 = 1

CONTIKI=../../../../..

MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/lib/json
MODULES_REL += ..

MODULES += os/net/app-layer/coap

include $(CONTIKI)/Makefile.include

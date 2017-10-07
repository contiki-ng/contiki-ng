CONTIKI_PROJECT = dr1175-node
all: $(CONTIKI_PROJECT)

TARGET ?= jn516x
JN516x_WITH_DR1175 = 1

CONTIKI=../../../../..

MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/lib/json
PROJECTDIRS += .. ../tools
PROJECT_SOURCEFILES += rpl-tools.c

CFLAGS += -DWITH_COAP
MODULES += os/net/app-layer/coap

include $(CONTIKI)/Makefile.include

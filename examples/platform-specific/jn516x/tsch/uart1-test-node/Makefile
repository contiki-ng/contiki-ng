CONTIKI_PROJECT = uart1-test-node

TARGET = jn516x
JN516x_WITH_DR1174 = 1
TARGET_WITH_UART1 = 1

CONTIKI=../../../../..

PROJECTDIRS += .. ../tools
PROJECT_SOURCEFILES += rpl-tools.c

CFLAGS += -DWITH_COAP
CFLAGS += -DREST=coap_rest_implementation
MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/net/app-layer/coap
MODULES += os/services/orchestra os/lib/json

all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include

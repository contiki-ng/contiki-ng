CONTIKI_PROJECT = uart1-test-node

PLATFORMS_ONLY = jn516x

TARGET = jn516x
JN516x_WITH_DR1174 = 1
TARGET_WITH_UART1 = 1

CONTIKI=../../../../..

MODULES_REL += ..

MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/net/app-layer/coap
MODULES += os/services/orchestra os/lib/json

all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include

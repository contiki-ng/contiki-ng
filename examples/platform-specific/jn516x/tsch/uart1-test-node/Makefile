CONTIKI_PROJECT = uart1-test-node

PLATFORMS_ONLY = jn516x

TARGET = jn516x
JN516x_WITH_DR1174 = 1
TARGET_WITH_UART1 = 1

CONTIKI=../../../../..

MODULES_REL += ..

MAKE_MAC = MAKE_MAC_TSCH
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap
MODULES += $(CONTIKI_NG_SERVICES_DIR)/orchestra $(CONTIKI_NG_LIB_DIR)/json

all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include

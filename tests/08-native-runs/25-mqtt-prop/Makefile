CONTIKI_PROJECT = test-mqtt-prop
all: $(CONTIKI_PROJECT)

TARGET ?= native

CONTIKI = ../../..

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/mqtt
MODULES += os/services/unit-test

include $(CONTIKI)/Makefile.include

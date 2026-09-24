CONTIKI_PROJECT = test-lwm2m-tlv
all: $(CONTIKI_PROJECT)

TARGET = native

# Only the TLV codec is under test. Pulling in the whole lwm2m module would
# bring the engine, and with it CoAP and the rest of the stack, none of which
# this test exercises.
PROJECTDIRS += $(CONTIKI)/os/services/lwm2m
PROJECTDIRS += $(CONTIKI)/os/net/app-layer/coap
PROJECT_SOURCEFILES += lwm2m-tlv.c

MODULES += os/services/unit-test

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include

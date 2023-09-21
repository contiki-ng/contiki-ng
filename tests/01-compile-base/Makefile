CURDIR := $(abspath $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST)))))

ifeq ($(CI), true)
  TOOLS_TEST ?= 1
else
  Q ?= @
endif

ifneq ($(TOOLS_TEST),1)
  CLANG_WARNINGS=1
  BINARY_SIZE_LOGFILE = $(CURDIR)/sizes.log
endif

TOOLS_EXAMPLESDIR = ../../tools

TOOLS_EXAMPLES = \
serial-io/native

ifeq ($(TOOLS_TEST), 1)
preparation:
	$(Q)$(MAKE) EXAMPLES="$(TOOLS_EXAMPLES)" EXAMPLESDIR=$(TOOLS_EXAMPLESDIR) all
	$(Q)$(MAKE) TOOLS_TEST=0 all
endif

EXAMPLESDIR=../../examples

EXAMPLES = \
6tisch/simple-node/z1:MAKE_WITH_PERIODIC_ROUTES_PRINT=1 \
hello-world/native \
hello-world/native:DEFINES=UIP_CONF_UDP=0 \
hello-world/native:MAKE_NET=MAKE_NET_NULLNET \
hello-world/native:MAKE_ROUTING=MAKE_ROUTING_RPL_CLASSIC \
hello-world/z1 \
storage/eeprom-test/native \
libs/logging/native \
libs/data-structures/native \
libs/stack-check/sky \
lwm2m-ipso-objects/native:MAKE_WITH_DTLS=1 \
lwm2m-ipso-objects/native:DEFINES=LWM2M_Q_MODE_CONF_ENABLED=1,LWM2M_Q_MODE_CONF_INCLUDE_DYNAMIC_ADAPTATION=1 \
rpl-border-router/native \
rpl-border-router/native:MAKE_ROUTING=MAKE_ROUTING_RPL_CLASSIC \
rpl-border-router/sky \
slip-radio/sky \
nullnet/native \
nullnet/sky:MAKE_MAC=MAKE_MAC_TSCH \
mqtt-client/native \
coap/coap-example-client/native \
coap/coap-example-server/native \
coap/coap-plugtest-server/native \
dev/dht11/native \
dev/dht11/sky \
dev/dht11/z1 \
snmp-server/native \
snmp-server/sky \
snmp-server/z1 \

include ../Makefile.compile-test

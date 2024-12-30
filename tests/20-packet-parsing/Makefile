CURDIR := $(abspath $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST)))))
HOST_OS := $(shell uname)

ifneq ($(HOST_OS),Darwin)
  BINARY_SIZE_LOGFILE = $(CURDIR)/sizes.log
endif

RUN_FILE = 1

EXAMPLESDIR = ./

EXAMPLES = \
packet-injector/native:./01-test-uip.sh \
packet-injector/native:./02-test-sicslowpan.sh \
packet-injector/native:./03-test-ble-l2cap.sh \
packet-injector/native:./04-test-tcpip.sh \

include ../Makefile.compile-test

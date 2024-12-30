CURDIR := $(abspath $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST)))))
BINARY_SIZE_LOGFILE = $(CURDIR)/sizes.log

RUN_FILE = 1

EXAMPLESDIR = ../../examples

EXAMPLES = \
rpl-udp/cc2538dk:./01-rpl-udp.sh

include ../Makefile.compile-test

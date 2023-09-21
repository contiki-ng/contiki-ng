CURDIR := $(abspath $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST)))))
HOST_OS := $(shell uname)

ifneq ($(HOST_OS),Darwin)
  BINARY_SIZE_LOGFILE = $(CURDIR)/sizes.log
endif

RUN_FILE = 1

EXAMPLESDIR = ./

EXAMPLES = \
nbr-multi-addrs/native:nbr-multi-addrs/build/native/test.native

include ../Makefile.compile-test

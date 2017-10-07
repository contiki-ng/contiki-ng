all: test-panid-handling test-tcsh-create-packet

MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/services/unit-test

TEST_CONFIG_TYPE ?= DEFAULT

ifeq ($(TEST_CONFIG_TYPE), SECURITY_ON)
CFLAGS += -D WITH_SECURITY_ON=1
endif

ifeq ($(TEST_CONFIG_TYPE), ALL_ENABLED)
CFLAGS += -D WITH_ALL_ENABLED=1
endif

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include

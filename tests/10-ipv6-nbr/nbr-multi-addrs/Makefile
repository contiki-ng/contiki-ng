CONTIKI_PROJECT = test
all: $(CONTIKI_PROJECT)

CFLAGS += -DUNIT_TEST_PRINT_FUNCTION=my_test_print
CFLAGS += -DLOG_CONF_LEVEL_IPV6=LOG_LEVEL_DBG
CFLAGS += -DNBR_TABLE_FIND_REMOVABLE=my_always_return_null
CFLAGS += -DUIP_DS6_NBR_CONF_MULTI_IPV6_ADDRS=1

PLATFORM_ONLY = native
TARGET = native
MODULES += os/sys/log os/services/unit-test

CONTIKI = ../../../
include $(CONTIKI)/Makefile.include

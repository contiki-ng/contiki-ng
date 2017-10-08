all:

APPS    += unit-test
MODULES += os/net/mac/tsch/sixtop

PROJECT_SOURCEFILES += common.c

ifeq ($(TEST_04),1)
CFLAGS  += -DSIXP_MSG_API_TEST=1
endif

CONTIKI = ../../..
# MAC layer set as TSCH even though we actually use test_mac_driver
MAKE_MAC = MAKE_MAC_TSCH
include $(CONTIKI)/Makefile.include

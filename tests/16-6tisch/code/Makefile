all:

APPS    += unit-test
MODULES += os/net/mac/tsch os/net/mac/tsch/sixtop

PROJECT_SOURCEFILES += common.c

ifeq ($(TEST_04),1)
CFLAGS  += -DSIXP_MSG_API_TEST=1
endif

CONTIKI = ../../..

include $(CONTIKI)/Makefile.include

all:

CFLAGS  += -D PROJECT_CONF_H=\"project-conf.h\"
MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/net/mac/tsch/sixtop os/services/unit-test

PROJECT_SOURCEFILES += common.c

CONTIKI = ../../..
MAKE_MAC = MAKE_MAC_TSCH
include $(CONTIKI)/Makefile.include

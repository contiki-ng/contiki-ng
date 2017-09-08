all:

CFLAGS  += -D PROJECT_CONF_H=\"project-conf.h\"
MODULES += os/net/mac/tsch os/net/mac/tsch/sixtop os/services/unit-test

PROJECT_SOURCEFILES += common.c

CONTIKI = ../../..
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include

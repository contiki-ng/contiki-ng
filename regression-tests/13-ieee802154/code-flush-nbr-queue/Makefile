all:

CFLAGS  += -D PROJECT_CONF_H=\"project-conf.h\"
APPS    += unit-test
MODULES += os/net/mac/tsch os/net/mac/tsch/sixtop

PROJECT_SOURCEFILES += common.c

CONTIKI = ../../..
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include

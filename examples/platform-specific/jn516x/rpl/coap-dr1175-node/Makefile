CONTIKI_PROJECT = dr1175-node
all: $(CONTIKI_PROJECT)

TARGET ?= jn516x
JN516x_WITH_DR1175 = 1

CONTIKI=../../../../..

CONTIKI_WITH_IPV6 = 1

MODULES += os/net/mac/tsch os/lib/json
PROJECTDIRS += .. ../tools
PROJECT_SOURCEFILES += rpl-tools.c
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CFLAGS += -DWITH_COAP
CFLAGS += -DUIP_CONF_TCP=0
MODULES += os/net/app-layer/coap

include $(CONTIKI)/Makefile.include

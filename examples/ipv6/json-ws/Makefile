CONTIKI=../../..

PROJECT_SOURCEFILES += json-ws.c

ifdef WITH_COSM
 CFLAGS += -DWITH_COSM=1
endif

ifdef WITH_UDP
 CFLAGS += -DWITH_UDP=1
 PROJECT_SOURCEFILES += json-ws-udp.c
endif

MODULES += os/lib/json os/net/app-layer/httpd-ws

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

ifeq ($(TARGET),)
 -include Makefile.target
endif

ifneq ($(TARGET),)
all: websense-$(TARGET)
endif

CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include

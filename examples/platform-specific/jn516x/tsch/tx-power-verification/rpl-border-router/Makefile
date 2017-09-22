CONTIKI_PROJECT=rpl-border-router

TARGET ?= jn516x
JN516x_WITH_DONGLE = 1

CONTIKI=../../../../../..

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += slip-bridge.c slip.c

PROJECTDIRS += .. ../../tools
PROJECT_SOURCEFILES += rpl-tools.c

CFLAGS += -DWITH_COAP
CFLAGS += -DUIP_CONF_TCP=0
MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/net/app-layer/coap
MODULES += os/services/orchestra os/lib/json

all: $(CONTIKI_PROJECT)
include $(CONTIKI)/Makefile.include

ifeq ($(PREFIX),)
 PREFIX = aaaa::1/64
endif

#no flow control
connect-router:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -v1 -B 1000000 $(PREFIX)

#using XON/XOFF flow control
connect-router-sw:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -v1 -X -B 1000000 $(PREFIX)

#using hw flow control
connect-router-hw:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -v1 -H -B 1000000 $(PREFIX)

#using no flow control
connect-router-no:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -v1 -B 1000000 $(PREFIX)

connect-router-cooja:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -a 127.0.0.1 $(PREFIX)

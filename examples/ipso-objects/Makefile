CONTIKI_PROJECT = example-ipso-objects

CONTIKI_SOURCEFILES += serial-protocol.c example-ipso-temperature.c

PLATFORMS_EXCLUDE = sky

all: $(CONTIKI_PROJECT)

MODULES += os/net/app-layer/coap
MODULES += os/services/lwm2m
MODULES += os/services/ipso-objects

CONTIKI=../..
include $(CONTIKI)/Makefile.include

# border router rules
$(CONTIKI)/tools/tunslip6:	$(CONTIKI)/tools/tunslip6.c
	(cd $(CONTIKI)/tools && $(MAKE) tunslip6)

connect-router:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 aaaa::1/64

connect-router-cooja:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -a 127.0.0.1 -p 60001 aaaa::1/64

connect-router-native:	$(CONTIKI)/examples/native-border-router/border-router.native
	sudo $(CONTIKI)/examples/native-border-router/border-router.native -a 127.0.0.1 -p 60001 aaaa::1/64

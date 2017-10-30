CONTIKI_PROJECT=border-router
all: $(CONTIKI_PROJECT)

CONTIKI=../..

ifeq ($(TARGET),)
  -include Makefile.target
endif

### Optionally, the target can add its own Makefile, to do things like e.g.
### add more source files to the build or define make variables.
-include $(TARGET)/Makefile.$(TARGET)

PROJECTDIRS += $(TARGET)
PROJECT_SOURCEFILES += slip-bridge.c httpd-simple.c

ifeq ($(PREFIX),)
 PREFIX = fd00::1/64
endif

include $(CONTIKI)/Makefile.include

$(CONTIKI)/tools/tunslip6:	$(CONTIKI)/tools/tunslip6.c
	(cd $(CONTIKI)/tools && $(MAKE) tunslip6)

connect-router:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 $(PREFIX)

connect-router-cooja:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -a 127.0.0.1 $(PREFIX)

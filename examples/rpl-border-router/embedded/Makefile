### Optionally, the target can add its own Makefile, to do things like e.g.
### add more source files to the build or define make variables.
-include $(SOURCES_DIR)/$(TARGET)/Makefile.$(TARGET)

PROJECTDIRS += $(SOURCES_DIR)/$(TARGET)

PROJECT_SOURCEFILES += slip-bridge.c httpd-simple.c

$(CONTIKI)/tools/tunslip6:	$(CONTIKI)/tools/tunslip6.c
	(cd $(CONTIKI)/tools && $(MAKE) tunslip6)

connect-router:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 $(PREFIX)

connect-router-cooja:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -a 127.0.0.1 $(PREFIX)

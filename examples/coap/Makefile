all: coap-example-server coap-example-client
# use target "plugtest-server" explicitly when required

CONTIKI=../..

# build RESTful resources
include $(CONTIKI)/Makefile.identify-target
ifeq ($(TARGET),native)
	PROJECTDIRS += ./resources-plugtest
endif
PROJECTDIRS += ./resources

# Include the CoAP implementation
MODULES += os/net/app-layer/coap

# optional rules to get assembly
#CUSTOM_RULE_C_TO_OBJECTDIR_O = 1
#CUSTOM_RULE_S_TO_OBJECTDIR_O = 1

include $(CONTIKI)/Makefile.include

# optional rules to get assembly
#$(OBJECTDIR)/%.o: asmdir/%.S
#	$(CC) $(CFLAGS) -MMD -c $< -o $@
#	@$(FINALIZE_DEPENDENCY)
#
#asmdir/%.S: %.c
#	$(CC) $(CFLAGS) -MMD -S $< -o $@

# border router rules
$(CONTIKI)/tools/tunslip6:	$(CONTIKI)/tools/tunslip6.c
	(cd $(CONTIKI)/tools && $(MAKE) tunslip6)

connect-router:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 fd00::1/64

connect-router-cooja:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -a 127.0.0.1 -p 60001 fd00::1/64

connect-router-native:	$(CONTIKI)/examples/native-border-router/border-router.native
	sudo $(CONTIKI)/examples/native-border-router/border-router.native -a 127.0.0.1 -p 60001 fd00::1/64

connect-minimal:
	sudo ip address add fdfd::1/64 dev tap0

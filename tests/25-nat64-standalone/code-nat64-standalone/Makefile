CONTIKI_PROJECT = nat64-standalone-test
all: $(CONTIKI_PROJECT)

CONTIKI = ../../..

# The standalone NAT64 module makes the node translate its own NAT64 traffic
# over host sockets, enables NAT64 at compile time, and suppresses the default
# route, so the probe needs no run-time flags. Native target only.
ifeq ($(TARGET),native)
MODULES += os/services/nat64/native/standalone
endif

include $(CONTIKI)/Makefile.include

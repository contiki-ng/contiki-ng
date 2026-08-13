CONTIKI_PROJECT = nat64-dns-client
all: $(CONTIKI_PROJECT)

CONTIKI = ../..

# Include the DNS resolver and HTTP socket modules.
MODULES += os/services/resolv
MODULES += os/net/app-layer/http-socket

include $(CONTIKI)/Makefile.include

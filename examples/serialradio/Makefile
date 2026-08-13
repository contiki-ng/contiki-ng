CONTIKI_PROJECT = serial-radio
all: $(CONTIKI_PROJECT)

# Project source files
PROJECT_SOURCEFILES += sniffer-mac.c

# The actual RX MAC is the sniffer driver (selected in project-conf.h); CSMA is
# pulled in for TX support. nullnet is the network driver. Note we keep the
# default (IPv6) MAKE_NET rather than MAKE_NET_NULLNET, because os/dev/slip.c —
# used here for the host framing — stores received bytes in uip_buf, which the
# IPv6 build provides.
MAKE_MAC = MAKE_MAC_CSMA
MODULES += os/net/nullnet

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

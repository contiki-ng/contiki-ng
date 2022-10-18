CONTIKI_PROJECT = snmp-server 
all: $(CONTIKI_PROJECT)

MODULES += os/net/app-layer/snmp

MODULES_REL += ./resources

NEEDS_CONTIKI_VERSION_FILES += snmp-SNMP-MIB-2-System.c

CONTIKI = ../..

include $(CONTIKI)/Makefile.include

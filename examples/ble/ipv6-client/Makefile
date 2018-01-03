DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT=client

all: $(CONTIKI_PROJECT)

upload:
	srfprog -t 'soc(XDS-L1000130)' -e all -p 'epfw(0)' -v rb -f $(CONTIKI_PROJECT).hex -a 0x0

MAKE_MAC = 		 MAKE_MAC_BLE
MAKE_NET =     MAKE_NET_IPV6
CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
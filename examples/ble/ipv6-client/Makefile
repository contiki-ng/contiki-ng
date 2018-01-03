DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT=client

all: $(CONTIKI_PROJECT)

upload:
	srfprog -t 'soc(XDS-L1000130)' -e all -p 'epfw(0)' -v rb -f $(CONTIKI_PROJECT).hex -a 0x0

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
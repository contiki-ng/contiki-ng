CONTIKI_PROJECT = udp-client udp-server
all: $(CONTIKI_PROJECT)

.PHONY: renode
renode: all
ifneq ($(TARGET),cc2538dk)
	$(error Only the cc2538dk TARGET is supported for Renode demo scripts)
endif
ifndef SCRIPT
	$(warning SCRIPT not defined! Using "rpl-udp.resc" as default)
	renode rpl-udp.resc
else
ifeq ($(wildcard $(SCRIPT)),)
	$(error SCRIPT "$(SCRIPT)" does not exist!)
endif
	renode $(SCRIPT)
endif

CONTIKI=../..
include $(CONTIKI)/Makefile.include

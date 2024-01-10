CONTIKI = ../../../..

all:
	$(MAKE) -C secure TOP=$(CONTIKI)
	$(MAKE) -C nonsecure TOP=$(CONTIKI)

%:
	$(MAKE) $@ -C secure TOP=$(CONTIKI)
	$(MAKE) $@ -C nonsecure TOP=$(CONTIKI)

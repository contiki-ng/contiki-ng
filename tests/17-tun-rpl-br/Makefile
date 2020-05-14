CONTIKI=../..

all: cooja

cooja: $(CONTIKI)/tools/cooja/dist/cooja.jar

$(CONTIKI)/tools/cooja/dist/cooja.jar:
	(cd $(CONTIKI)/tools/cooja; ant jar)

include ../Makefile.script-test

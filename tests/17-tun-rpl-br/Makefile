CONTIKI=../..

.PHONY: start

start:
	@ant -e -logger org.apache.tools.ant.listener.SimpleBigProjectLogger -f $(CONTIKI)/tools/cooja/build.xml jar
	@$(MAKE) all

include ../Makefile.script-test

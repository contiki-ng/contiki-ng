CONTIKI_PROJECT = gpio-hal-example
CONTIKI = ../../..

PLATFORMS_ONLY += cc26x0-cc13x0 cc2538dk openmote zoul simplelink nrf52840
PLATFORMS_ONLY += cooja native

include $(CONTIKI)/Makefile.identify-target

MODULES_REL += $(TARGET)

all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include

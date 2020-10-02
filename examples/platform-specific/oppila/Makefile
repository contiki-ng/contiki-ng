CONTIKI_PROJECT =  test-bmp180 test-grove-light-sensor test-adxl345.c

CONTIKI_TARGET_SOURCEFILES += bmp180.c adc-sensors.c adxl345.c

all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY = oppila 

CONTIKI = ../../..

include $(CONTIKI)/Makefile.dir-variables

include $(CONTIKI)/Makefile.include

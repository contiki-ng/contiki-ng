CONTIKI_PROJECT = touch
all: $(CONTIKI_PROJECT)

CONTIKI = ../../..

MODULES += $(CONTIKI_NG_DRIVERS_SENSOR_DIR)/hall

# Platforms missing gpio-hal.h
PLATFORMS_EXCLUDE += sky z1

# Platforms missing US_TO_RTIMERTICKS and RTIMERTICKS_TO_US
PLATFORMS_EXCLUDE += native 

include $(CONTIKI)/Makefile.include
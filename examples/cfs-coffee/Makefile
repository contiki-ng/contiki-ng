DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI = ../..

all: test-cfs test-coffee example-coffee

ifeq ($(TARGET),avr-raven)
  COFFEE_FILES = 4
endif

include $(CONTIKI)/Makefile.include

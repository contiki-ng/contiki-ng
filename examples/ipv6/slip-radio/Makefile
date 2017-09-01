CONTIKI_PROJECT=slip-radio
all: $(CONTIKI_PROJECT)
MODULES += os/services/slip-cmd

ifeq ($(TARGET),)
  -include Makefile.target
endif

CONTIKI=../../..

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += slip-net.c no-framer.c
ifeq ($(TARGET),sky)
  PROJECT_SOURCEFILES += slip-radio-cc2420.c slip-radio-sky-sensors.c
endif
ifeq ($(TARGET),nooliberry)
  PROJECT_SOURCEFILES += slip-radio-rf230.c
endif

CONTIKI_WITH_RPL = 0
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include

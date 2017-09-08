CONTIKI_PROJECT=border-router
all: $(CONTIKI_PROJECT)
MODULES += os/services/slip-cmd

CONTIKI=../../..

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += border-router-cmds.c tun-bridge.c border-router-mac.c \
slip-config.c slip-dev.c

WITH_WEBSERVER=1
ifeq ($(WITH_WEBSERVER),1)
CFLAGS += -DWEBSERVER=1
PROJECT_SOURCEFILES += httpd-simple.c
else ifneq ($(WITH_WEBSERVER), 0)
MODULES += $(WITH_WEBSERVER)
CFLAGS += -DWEBSERVER=2
endif

# Custom MAC layer (border_router_mac_driver)
MAKE_MAC = MAKE_MAC_OTHER
include $(CONTIKI)/Makefile.include

connect-router:	border-router.native
	sudo ./border-router.native fd00::1/64

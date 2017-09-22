CONTIKI_PROJECT      = sixp-node.c
PROJECT_SOURCEFILES += test-sf.c

CONTIKI  = ../../../
CFLAGS  += -DPROJECT_CONF_H=\"project-conf.h\"

MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/net/mac/tsch/sixtop

include $(CONTIKI)/Makefile.include

CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

PLATFORMS_EXCLUDE = sky z1 nrf52dk native simplelink
BOARDS_EXCLUDE = srf06/cc13x0 launchpad/cc1310 launchpad/cc1350 sensortag/cc2650 sensortag/cc1350

MODULES_REL += ../testbeds
MODULES += os/services/deployment
MODULES += os/services/simple-energest

CONFIG?=CONFIG_TSCH_OPTIMS

ifeq ($(CONFIG),CONFIG_CSMA)
MAKE_MAC = MAKE_MAC_CSMA
else ifeq ($(CONFIG),CONFIG_TSCH)
MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/services/orchestra
else ifeq ($(CONFIG),CONFIG_TSCH_OPTIMS)
MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/services/orchestra
CFLAGS += -DCONFIG_OPTIMS=1
else ifeq ($(CONFIG),CONFIG_TSCH_OPTIMS2)
MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/services/orchestra
CFLAGS += -DCONFIG_OPTIMS=2
endif

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include

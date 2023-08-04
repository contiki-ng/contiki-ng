CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

PLATFORMS_EXCLUDE = sky native

CONTIKI=../../..

 # force Orchestra from command line
MAKE_WITH_ORCHESTRA ?= 0
# force Security from command line
MAKE_WITH_SECURITY ?= 0
 # print #routes periodically, used for regression tests
MAKE_WITH_PERIODIC_ROUTES_PRINT ?= 0
# RPL storing mode?
MAKE_WITH_STORING_ROUTING ?= 0
# Orchestra link-based rule? (Works only if Orchestra & storing mode routing is enabled)
MAKE_WITH_LINK_BASED_ORCHESTRA ?= 0
# Use the Orchestra root rule?
MAKE_WITH_ORCHESTRA_ROOT_RULE ?= 0

MAKE_MAC = MAKE_MAC_TSCH

include $(CONTIKI)/Makefile.dir-variables
include $(CONTIKI)/Makefile.identify-target
ifneq ($(TARGET),z1)
MODULES += $(CONTIKI_NG_SERVICES_DIR)/shell
endif

ORCHESTRA_EXTRA_RULES = &unicast_per_neighbor_rpl_ns

ifeq ($(MAKE_WITH_ORCHESTRA),1)
  MODULES += $(CONTIKI_NG_SERVICES_DIR)/orchestra

  ifeq ($(MAKE_WITH_STORING_ROUTING),1)
    ifeq ($(MAKE_WITH_LINK_BASED_ORCHESTRA),1)
      # enable the `link_based` rule
      ORCHESTRA_EXTRA_RULES = &unicast_per_neighbor_link_based
    else
      # enable the `rpl_storing` rule
      ORCHESTRA_EXTRA_RULES = &unicast_per_neighbor_rpl_storing
    endif

  else
    ifeq ($(MAKE_WITH_LINK_BASED_ORCHESTRA),1)
      $(error "Inconsistent configuration: link-based Orchestra requires routing info")
    endif

  endif

  ifeq ($(MAKE_WITH_ORCHESTRA_ROOT_RULE),1)
    # add the root rule
    ORCHESTRA_EXTRA_RULES +=,&special_for_root
  endif

  # pass the Orchestra rules to the compiler
  CFLAGS += -DORCHESTRA_CONF_RULES="{&eb_per_time_source,$(ORCHESTRA_EXTRA_RULES),&default_common}"
endif

ifeq ($(MAKE_WITH_STORING_ROUTING),1)
  MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC
  CFLAGS += -DRPL_CONF_MOP=RPL_MOP_STORING_NO_MULTICAST
endif

ifeq ($(MAKE_WITH_SECURITY),1)
CFLAGS += -DWITH_SECURITY=1
endif

ifeq ($(MAKE_WITH_PERIODIC_ROUTES_PRINT),1)
CFLAGS += -DWITH_PERIODIC_ROUTES_PRINT=1
endif

include $(CONTIKI)/Makefile.include

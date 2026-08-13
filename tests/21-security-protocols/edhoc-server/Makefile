CONTIKI_PROJECT = edhoc-test-server

all: $(CONTIKI_PROJECT)

# Credential type selection - default to 'default' if not specified
CREDS_TYPE ?= default

# Map credential types to preprocessor definitions
ifeq ($(CREDS_TYPE),interop-sign)
  CFLAGS += -DMAKE_EDHOC_CREDS_INTEROP_SIGN=1
else ifeq ($(CREDS_TYPE),interop-dh)
  CFLAGS += -DMAKE_EDHOC_CREDS_INTEROP_DH=1
else ifeq ($(CREDS_TYPE),interop-ca)
  CFLAGS += -DMAKE_EDHOC_CREDS_INTEROP_CA=1
else ifeq ($(CREDS_TYPE),rfc9529-static-dh)
  CFLAGS += -DMAKE_EDHOC_CREDS_RFC9529_STATIC_DH=1
else ifneq ($(CREDS_TYPE),default)
  $(error Invalid CREDS_TYPE: $(CREDS_TYPE). Valid options: default, interop-sign, interop-dh, interop-ca, rfc9529-static-dh)
endif

# Handle direct make variable for RFC 9529 Static DH (for backward compatibility)
ifdef MAKE_EDHOC_CREDS_RFC9529_STATIC_DH
  CFLAGS += -DMAKE_EDHOC_CREDS_RFC9529_STATIC_DH=1
endif

# Add information about selected credential type to build
CFLAGS += -DCREDS_TYPE_STRING=\"$(CREDS_TYPE)\"


#uECC fast execution optimization
CFLAGS += -DuECC_OPTIMIZATION_LEVEL=4
CFLAGS += -DuECC_SQUARE_FUNC=1

CONTIKI = ../../..
include $(CONTIKI)/Makefile.dir-variables

MODULES += $(CONTIKI_NG_SECURITY_DIR)/edhoc
MODULES += $(CONTIKI_NG_SECURITY_DIR)/edhoc/server

include $(CONTIKI)/Makefile.include

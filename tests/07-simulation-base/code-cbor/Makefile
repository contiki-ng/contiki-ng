all: test-cbor

MODULES += os/services/unit-test
MODULES += $(CONTIKI_NG_LIB_DIR)/cbor

MAKE_MAC = MAKE_MAC_NULLMAC
MAKE_NET = MAKE_NET_NULLNET

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include

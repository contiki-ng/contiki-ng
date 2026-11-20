CONTIKI_PROJECT = compress ecdh ecdsa
all: $(CONTIKI_PROJECT)

CONTIKI = ../../..
include $(CONTIKI)/Makefile.identify-target

# The lib/ecc.h ECC service. Uses the portable software (uECC) backend by
# default on every target.
MODULES += os/services/ecc

# Hardware acceleration is opt-in and selected at build time (the backend is
# chosen at link time, so it is not a runtime switch). On a platform that ships
# a hardware lib/ecc.h backend, set ECC_HW_BACKEND to route these examples onto
# it -- e.g. the nRF52840 CryptoCell-310:
#
#   make TARGET=nrf52840 BOARD=dk ECC_HW_BACKEND=cc310 \
#        NRF_CC310_SDK=/path/to/nRF5_SDK_17.1.0_ddde560
#
# Leave ECC_HW_BACKEND unset for the software backend (the default, all targets).
# The examples call ecc_session_begin()/ecc_session_end() (no-ops in software),
# so the identical source runs with or without hardware acceleration; the chosen
# backend is printed at startup ("ECC backend: ...").
ifeq ($(ECC_HW_BACKEND),cc310)
  CFLAGS += -DECC_BACKEND_LABEL='"CC310 hardware"'
else
  CFLAGS += -DECC_BACKEND_LABEL='"software (uECC)"'
endif

include $(CONTIKI)/Makefile.include

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
  # The nRF52840 CC310 backend routes lib/ecc.h onto nrfxlib's cc3xx PSA driver,
  # which calls back into the mbedTLS PSA crypto core (RNG, allocator, hash
  # dispatch). Link that core (tf-psa-crypto). MODULES must be added here (not in
  # the arch Makefile.cc310-crypto, whose MODULES would be expanded too late).
  MBEDTLS_PATH := os/net/security/mbedtls
  CFLAGS += -DMBEDTLS_CONFIG_FILE=\"arch/cpu/nrf/lib/nrf_cc310/mbedtls-config-cc310.h\"
  MODULES += $(MBEDTLS_PATH) $(addprefix $(MBEDTLS_PATH)/,library include)
  MODULES += $(addprefix $(MBEDTLS_PATH)/tf-psa-crypto/,core include)
  MODULES += $(addprefix $(MBEDTLS_PATH)/tf-psa-crypto/drivers/builtin/,src include)
  # These example programs (ecdsa/ecdh) share object basenames with mbedTLS's
  # library/ecdsa.c and library/ecdh.c. Contiki resolves the colliding basename
  # to the example source (vpath precedence), dragging its AUTOSTART main into
  # every target's link (multiple definition of autostart_processes). Exclude the
  # mbedTLS ones -- unused here anyway: the PSA config disables MBEDTLS_ECDSA_C/
  # MBEDTLS_ECDH_C and the CC310 does the EC math.
  MODULES_SOURCES_EXCLUDES += ecdsa.c ecdh.c
else
  CFLAGS += -DECC_BACKEND_LABEL='"software (uECC)"'
endif

include $(CONTIKI)/Makefile.include

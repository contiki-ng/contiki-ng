CONTIKI_PROJECT = flpr-host
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../..
TARGET ?= nrf
BOARD ?= nrf54l15/dk

# The M33 loader shares the FLPR memory map (flpr-shared.h). Point only at the
# dedicated shared/ dir — NOT the whole FLPR CPU dir, which would shadow the
# M33's own headers (e.g. rtimer-arch.h).
CFLAGS += -I$(CONTIKI)/arch/cpu/nrf-vpr/shared

# Board-dependent LED wiring:
#  - The FLPR drives the user LED on GPIO port 2; the pin differs per board and
#    is passed through to the FLPR sub-make below as LED0_PIN.
#  - The M33 blinks a second LED only on boards that have one (the XIAO has a
#    single user LED, driven by the FLPR), gated by FLPR_HOST_M33_LED.
ifeq ($(BOARD),nrf54l15/xiao)
  FLPR_LED_PIN := 0             # XIAO user LED = P2.00, active low (FLPR-driven)
  FLPR_LED_ACTIVE_LOW := 1
else
  FLPR_LED_PIN := 9             # nRF54L15-DK LED0 = P2.09, active high
  FLPR_LED_ACTIVE_LOW := 0
  CFLAGS += -DFLPR_HOST_M33_LED # DK has a second LED (gpio1.10) for the M33
endif

# Sub-project: the FLPR firmware that gets embedded in this M33 image.
FLPR_DIR        := $(CONTIKI)/examples/platform-specific/nrf/hello-vpr
FLPR_BIN        := $(FLPR_DIR)/build/nrf-vpr/hello-vpr.bin
FLPR_BLOB_GEN   := $(CONTIKI)/tools/flpr-blob-gen.py

# Build the FLPR firmware and (re)generate flpr-blob.h before Contiki's
# pattern rules try to compile flpr-host.c. Runs at make-load time so the
# header exists by the time the .c is compiled. Skipped for clean targets.
# BOARD= unsets the inherited M33 BOARD when invoking the FLPR sub-make.
# The cmp/mv dance avoids touching flpr-blob.h's mtime when content is
# unchanged, so flpr-host.c only rebuilds when the FLPR firmware actually
# changed.
ifeq ($(filter clean distclean,$(MAKECMDGOALS)),)
  $(info nrf-vpr: ensuring FLPR firmware is up to date...)
  FLPR_BUILD_LOG := $(shell $(MAKE) --no-print-directory -C $(FLPR_DIR) \
                              TARGET=nrf-vpr BOARD= WERROR=0 \
                              LED0_PIN=$(FLPR_LED_PIN) \
                              LED0_ACTIVE_LOW=$(FLPR_LED_ACTIVE_LOW) hello-vpr.bin 2>&1)
  ifneq ($(.SHELLSTATUS),0)
    $(error FLPR build failed:$(FLPR_BUILD_LOG))
  endif
  IGNORE := $(shell python3 $(FLPR_BLOB_GEN) $(FLPR_BIN) > flpr-blob.h.tmp && \
              if cmp -s flpr-blob.h.tmp flpr-blob.h 2>/dev/null; \
              then rm flpr-blob.h.tmp; \
              else mv flpr-blob.h.tmp flpr-blob.h; fi)
endif

CLEAN += flpr-blob.h flpr-blob.h.tmp

include $(CONTIKI)/Makefile.include

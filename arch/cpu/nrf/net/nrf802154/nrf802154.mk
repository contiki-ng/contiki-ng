# nrf_802154 radio integration for the nRF5340 network core.
#
# Included from Makefile.nrf5340_network when NRF_802154=1, AFTER Makefile.nrf
# (so CONTIKI_CPU_SOURCEFILES / CONTIKI_SOURCEFILES already contain the legacy
# nrf-ieee-driver-arch.c, which we replace here). Mirrors the nRF54L15 port's
# library wiring (arch/cpu/nrf/nrf54l15/Makefile.nrf54l15).

NRF_802154_ROOT := $(CONTIKI_CPU)/lib/sdk-nrfxlib/nrf_802154
NRF802154_GLUE  := $(CONTIKI_CPU)/net/nrf802154

# -- Replace the raw radio driver with the nrf_802154 wrapper. nrf-ieee-driver-
#    arch.c is pulled in both via CONTIKI_CPU_SOURCEFILES (Makefile.nrf) and via
#    the net/ module glob (MODULES_REL in the example), so exclude both. --
CONTIKI_CPU_SOURCEFILES := $(filter-out nrf-ieee-driver-arch.c,$(CONTIKI_CPU_SOURCEFILES))
CONTIKI_SOURCEFILES := $(filter-out nrf-ieee-driver-arch.c,$(CONTIKI_SOURCEFILES))
MODULES_SOURCES_EXCLUDES += nrf-ieee-driver-arch.c

# -- Project configuration + platform assert override. --
CFLAGS += -DNRF_802154_PROJECT_CONFIG=\"nrf_802154_project_config.h\"
CFLAGS += -DNRF_802154_PLATFORM_ASSERT_INCLUDE=\"nrf_802154_platform_assert.h\"

# -- The driver uses DPPI on the nRF5340. --
CFLAGS += -DNRFX_DPPI_ENABLED=1

# -- nrf_802154 acknowledges in hardware, so disable the IPC MAC software ACK. --
CFLAGS += -DNRF_IPC_MAC_CONF_HW_AUTOACK=1

# -- Include paths: our glue/config first, then the library. --
CFLAGS += -I$(NRF802154_GLUE)
CFLAGS += -I$(NRF_802154_ROOT)/common/include
CFLAGS += -I$(NRF_802154_ROOT)/driver/include
CFLAGS += -I$(NRF_802154_ROOT)/driver/include/platform
CFLAGS += -I$(NRF_802154_ROOT)/driver/src
CFLAGS += -I$(NRF_802154_ROOT)/driver/src/mac_features
CFLAGS += -I$(NRF_802154_ROOT)/driver/src/mac_features/ack_generator
CFLAGS += -I$(NRF_802154_ROOT)/sl/include
CFLAGS += -I$(NRF_802154_ROOT)/sl/include/platform
CFLAGS += -I$(NRF_802154_ROOT)/sl/include/rsch
CFLAGS += -I$(NRF_802154_ROOT)/sl/include/timer
CFLAGS += -I$(NRF_802154_ROOT)/sl/sl_opensource/include

# -- vpath dirs for the wrapper/glue and library sources. --
EXTERNALDIRS += $(NRF802154_GLUE)
EXTERNALDIRS += $(NRF_802154_ROOT)/common/src
EXTERNALDIRS += $(NRF_802154_ROOT)/driver/src
EXTERNALDIRS += $(NRF_802154_ROOT)/driver/src/mac_features
EXTERNALDIRS += $(NRF_802154_ROOT)/driver/src/mac_features/ack_generator
EXTERNALDIRS += $(NRF_802154_ROOT)/sl/sl_opensource/src

# -- Contiki-NG wrapper + platform abstraction. --
NRF802154_GLUE_SRCS  = nrf-ieee-driver-nrf53.c
NRF802154_GLUE_SRCS += nrf_802154_platform_sl_lptimer.c
NRF802154_GLUE_SRCS += nrf_802154_platform_clock.c
NRF802154_GLUE_SRCS += nrf_802154_platform_irq.c
NRF802154_GLUE_SRCS += nrf_802154_platform_hp_timer.c
NRF802154_GLUE_SRCS += nrf_802154_platform_timestamper.c
NRF802154_GLUE_SRCS += nrf_802154_platform_random.c
NRF802154_GLUE_SRCS += nrf_802154_platform_temperature.c
NRF802154_GLUE_SRCS += nrf_802154_platform_misc.c
CONTIKI_SOURCEFILES += $(NRF802154_GLUE_SRCS)

# -- nrf_802154 driver core. --
NRF_802154_DRV_SRC := $(NRF_802154_ROOT)/driver/src
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_core.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_core_hooks.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_critical_section.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_co.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_trx.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_trx_dppi.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_swi.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_notification_direct.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_notification_swi.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_request_direct.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_request_swi.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_swi_callouts_weak.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_pib.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_rx_buffer.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_tx_work_buffer.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_queue.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_rssi.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_tx_power.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_stats.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_debug.c
NRF_802154_C_SRCS += $(NRF_802154_DRV_SRC)/nrf_802154_debug_gpio.c

# -- MAC features. --
NRF_802154_MAC_SRC := $(NRF_802154_DRV_SRC)/mac_features
NRF_802154_C_SRCS += $(NRF_802154_MAC_SRC)/nrf_802154_filter.c
NRF_802154_C_SRCS += $(NRF_802154_MAC_SRC)/nrf_802154_frame_parser.c
NRF_802154_C_SRCS += $(NRF_802154_MAC_SRC)/nrf_802154_precise_ack_timeout.c
NRF_802154_C_SRCS += $(NRF_802154_MAC_SRC)/nrf_802154_security_pib_ram.c
NRF_802154_C_SRCS += $(NRF_802154_MAC_SRC)/nrf_802154_imm_tx.c
NRF_802154_C_SRCS += $(NRF_802154_MAC_SRC)/nrf_802154_tx_timestamp_provider.c
NRF_802154_C_SRCS += $(NRF_802154_MAC_SRC)/ack_generator/nrf_802154_ack_data.c
NRF_802154_C_SRCS += $(NRF_802154_MAC_SRC)/ack_generator/nrf_802154_ack_generator.c
NRF_802154_C_SRCS += $(NRF_802154_MAC_SRC)/ack_generator/nrf_802154_imm_ack_generator.c
NRF_802154_C_SRCS += $(NRF_802154_MAC_SRC)/ack_generator/nrf_802154_enh_ack_generator.c

# -- Open-source SL (nrf_802154_sl_timer.c is intentionally skipped -- the SL
#    timer service is provided by nrf_802154_platform_sl_lptimer.c). --
NRF_802154_SL_SRC := $(NRF_802154_ROOT)/sl/sl_opensource/src
NRF_802154_C_SRCS += $(NRF_802154_SL_SRC)/nrf_802154_sl_ant_div.c
NRF_802154_C_SRCS += $(NRF_802154_SL_SRC)/nrf_802154_sl_capabilities.c
NRF_802154_C_SRCS += $(NRF_802154_SL_SRC)/nrf_802154_sl_coex.c
NRF_802154_C_SRCS += $(NRF_802154_SL_SRC)/nrf_802154_sl_crit_sect_if.c
NRF_802154_C_SRCS += $(NRF_802154_SL_SRC)/nrf_802154_sl_fem.c
NRF_802154_C_SRCS += $(NRF_802154_SL_SRC)/nrf_802154_sl_log.c
NRF_802154_C_SRCS += $(NRF_802154_SL_SRC)/nrf_802154_sl_rsch.c

# -- Common utilities. --
NRF_802154_C_SRCS += $(NRF_802154_ROOT)/common/src/nrf_802154_common_utils.c

CONTIKI_SOURCEFILES += $(notdir $(NRF_802154_C_SRCS))

CONTIKI_PROJECT = ip64-router
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../..

PLATFORMS_ONLY = nrf

include $(CONTIKI)/Makefile.dir-variables

# The ENC28J60 driver plus its IP64 glue, and the SPI-HAL arch layer.
MODULES += $(CONTIKI_NG_DRIVERS_ETHERNET_DIR)/enc28j60

# SPIM22 is the nRF54L15 DK's expansion-header SPI. SPIM00 is taken by the
# on-board flash, and SPIM20 shares a SERIAL slot with the console UART.
NRF_WITH_SPI = 1
NRF_SPI_INSTANCES = 22

# Pull in os/services/ip64 and point uIP's fallback interface at it.
WITH_IP64 = 1
CFLAGS += -DUIP_FALLBACK_INTERFACE=ip64_uip_fallback_interface

include $(CONTIKI)/Makefile.include

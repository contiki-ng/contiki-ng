CONTIKI_PROJECT = enc28j60-test
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../..

PLATFORMS_ONLY = nrf

# Pull in the ENC28J60 driver and the SPI-HAL arch layer by name rather than
# adding the whole module: the module also contains the IP64 glue driver,
# which needs the IP64 service. This test deliberately stops at the chip.
PROJECTDIRS += $(CONTIKI)/arch/dev/ethernet/enc28j60
PROJECT_SOURCEFILES += enc28j60.c enc28j60-arch-spi-hal.c

# SPIM22 is the nRF54L15 DK's expansion-header SPI. SPIM00 is taken by the
# on-board flash, and SPIM20 shares a SERIAL slot with the console UART.
NRF_WITH_SPI = 1
NRF_SPI_INSTANCES = 22

include $(CONTIKI)/Makefile.include

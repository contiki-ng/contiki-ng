CONTIKI_PROJECT = spi-flash
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../..

PLATFORMS_ONLY = nrf

# Bring up the SPI HAL on SPIM00, which is where the nRF54L15 DK's onboard
# MX25R6435F flash sits.
NRF_WITH_SPI = 1
NRF_SPI_INSTANCES = 00

include $(CONTIKI)/Makefile.include

CONTIKI_PROJECT = spi-flash
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../..

PLATFORMS_ONLY = nrf

# Bring up the SPI HAL on SPIM00, which is where the nRF54L15 DK's onboard
# MX25R6435F flash sits.
# SPI is on by default with the SoC's default SPIM instance, which can drive
# the flash pins on every DK.

include $(CONTIKI)/Makefile.include

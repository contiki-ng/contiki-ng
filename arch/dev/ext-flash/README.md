# Ext-flash Driver

This is a generic driver for external SPI flash memories. It has been tested with the following parts:
- Winbond W25X20CL
- Winbond W25X40CL
- Macronix MX25R8035F
- Macronix MX25R1635F
- Macronix MX25R6435F
- Macronix MX25U6435F

These parts have identical instruction sets; only the manufacturer and device ID differ, which must be specified in `board.h`.
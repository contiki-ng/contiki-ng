/* Linker script for the nRF54L15 non-secure application image. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x0008A000, LENGTH = 0x000D3000
  RAM (rwx)  : ORIGIN = 0x20014000, LENGTH = 0x00014000
}

INCLUDE "nrf_common.ld"

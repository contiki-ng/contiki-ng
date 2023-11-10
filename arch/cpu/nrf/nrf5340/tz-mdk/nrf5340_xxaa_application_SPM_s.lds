/* Linker script to configure memory regions. */

#include "../partition/region_defs.h"

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = S_CODE_START, LENGTH = S_CODE_SIZE /*0x100000*/
  EXTFLASH (rx) : ORIGIN = 0x10000000, LENGTH = 0x8000000
  RAM (rwx) : ORIGIN = S_DATA_START, LENGTH = S_DATA_SIZE
}

#include "nrf_common.lds"

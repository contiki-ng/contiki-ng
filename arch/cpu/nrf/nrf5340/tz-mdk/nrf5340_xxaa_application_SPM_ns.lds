/* Linker script to configure memory regions. */

#include "../partition/region_defs.h"

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = NS_CODE_START, LENGTH = NS_CODE_SIZE /*0x100000*/
  EXTFLASH (rx) : ORIGIN = 0x10000000, LENGTH = 0x8000000
  RAM (rwx) : ORIGIN = NS_DATA_START, LENGTH = NS_DATA_SIZE
}

#include "nrf_common.lds"

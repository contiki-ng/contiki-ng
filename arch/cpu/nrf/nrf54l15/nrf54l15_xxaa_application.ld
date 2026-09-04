/*
 * Linker script for the nRF54L15 application core without TrustZone:
 * the MDK's nrf54l15_xxaa_application.ld with an optional region at the
 * top of the code memory reserved for xmem (MAKE_WITH_XMEM=1,
 * XMEM_CONF_SIZE), passed in as --defsym __xmem_conf_size. Without it
 * nothing is reserved.
 */

SEARCH_DIR(.)
GROUP(-lgcc -lc)

/* Code memory available to the application, inside global RRAM0. */
__rram_code_size = 0x17D000;

MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = __rram_code_size - (DEFINED(__xmem_conf_size) ? __xmem_conf_size : 0)
  RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 0x20000
  RAM1 (rwx) : ORIGIN = 0x20020000, LENGTH = 0x20000
}

__xmem_start = ORIGIN(FLASH) + LENGTH(FLASH);
__xmem_size = DEFINED(__xmem_conf_size) ? __xmem_conf_size : 0;

/* Otherwise LENGTH(FLASH) wraps and the region lands outside the RRAM. */
ASSERT(__xmem_size <= __rram_code_size, "XMEM_CONF_SIZE exceeds the code memory")

INCLUDE "nrf_common.ld"

/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = __flash, LENGTH = __flash_size
  RAM (rwx) :  ORIGIN = __ram, LENGTH = __ram_size
}

INCLUDE "nrf_common.ld"

/* These symbols are used by the stack check library. */
_stack = end;
_stack_origin = ORIGIN(RAM) + LENGTH(RAM);
_heap = _stack;
_eheap = _stack_origin;

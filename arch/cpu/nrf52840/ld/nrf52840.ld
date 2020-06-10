/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x0, LENGTH = 0x80000
  RAM (rwx) :  ORIGIN = 0x20000000, LENGTH = 0x10000
}

INCLUDE "nrf_common.ld"

/* These symbols are used by the stack check library. */
_stack = end;
_stack_origin = ORIGIN(RAM) + LENGTH(RAM);
_heap = _stack;
_eheap = _stack_origin;

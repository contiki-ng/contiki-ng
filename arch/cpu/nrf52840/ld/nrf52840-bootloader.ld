/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x1000, LENGTH = 0xff000
  RAM (rwx) :  ORIGIN = 0x20000008, LENGTH = 0x3fff8
}

INCLUDE "nrf_common.ld"

/* These symbols are used by the stack check library. */
_stack = end;
_stack_origin = ORIGIN(RAM) + LENGTH(RAM);
_heap = _stack;
_eheap = _stack_origin;

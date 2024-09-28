/* Linker script to configure memory regions. */

_ld_flash_start  = DEFINED(ld_flash_start)  ? ld_flash_start  : 0x00000000;
_ld_flash_length = DEFINED(ld_flash_length) ? ld_flash_length : 0x00080000;

_ld_ram_start    = DEFINED(ld_ram_start)    ? ld_ram_start    : 0x20000000;
_ld_ram_length   = DEFINED(ld_ram_length)   ? ld_ram_length   : 0x00018000;

_ld_stack_size   = DEFINED(ld_stack_size)   ? ld_stack_size   : 0x00000800;

MEMORY
{
  FLASH (rx) : ORIGIN = _ld_flash_start, LENGTH = _ld_flash_length - 0x1000
  RAM (rwx)  : ORIGIN = _ld_ram_start,   LENGTH = _ld_ram_length
}

/* Library configurations */
GROUP(libgcc.a libc.a libm.a libnosys.a)

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __Vectors_End
 *   __Vectors_Size
 */
ENTRY(Reset_Handler)

SECTIONS
{
	.text :
	{
		FILL(0xff)
		KEEP(*(.vectors))
		__Vectors_End = .;
		__Vectors_Size = __Vectors_End - __Vectors;

		. = ALIGN(4);

		KEEP(*(.init))
		KEEP(*(.fini))

		/* .ctors */
		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

		/* .dtors */
		*crtbegin.o(.dtors)
		*crtbegin?.o(.dtors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
		*(SORT(.dtors.*))
		*(.dtors)

		KEEP(*(.eh_frame*))
	} > FLASH

	.privileged_functions :
	{
		. = ALIGN(32);
		__privileged_functions_start__ = .;
		*(privileged_functions)
		. = ALIGN(32);
		__privileged_functions_end__ = . - 1;
	} > FLASH

	.freertos_system_calls :
	{
		. = ALIGN(32);
		__syscalls_flash_start__ = .;
		*(freertos_system_calls)
		. = ALIGN(32);
		__syscalls_flash_end__ = . - 1;
	} > FLASH

	.text :
	{
		. = ALIGN(32);
		__unprivileged_flash_start__ = .;
		*(.text*)
		*(.rodata*)

		. = ALIGN(32);
		__unprivileged_flash_end__ = . - 1;
	} > FLASH

	.ARM.extab :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > FLASH

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > FLASH
	__exidx_end = .;

	/* To copy multiple ROM to RAM sections,
	 * uncomment .copy.table section and,
	 * define __STARTUP_COPY_MULTIPLE in startup_ARMCMx.S */
	.copy.table :
	{
		. = ALIGN(4);
		__copy_table_start__ = .;
		LONG (__etext)
		LONG (__data_start__)
		LONG (__data_end__ - __data_start__)
		__copy_table_end__ = .;
	} > FLASH

	/* To clear multiple BSS sections,
	 * uncomment .zero.table section and,
	 * define __STARTUP_CLEAR_BSS_MULTIPLE in startup_ARMCMx.S */
	.zero.table :
	{
		. = ALIGN(4);
		__zero_table_start__ = .;
		LONG (__bss_start__)
		LONG (__bss_end__ - __bss_start__)
		__zero_table_end__ = .;
	} > FLASH

	__etext = .;

	._vtor (NOLOAD) :
	{
		*(vtor_ram);
	} > RAM

	/* startup_M2351.S defines stack section. to avoid name conflict, declare _stack section */
	._stack (NOLOAD) :
	{
		_stack = .;
		__StackLimit = .;
		. += _ld_stack_size;
		__StackTop = .;
		_stack_origin = .;
	} > RAM

	.data : AT (__etext)
	{
		FILL(0xff)
		__data_start__ = .;

		. = ALIGN(32);
		__privileged_sram_start__ = .;
		*(privileged_data);
		. = ALIGN(32);
		__privileged_sram_end__ = . - 1;

		*(vtable)
		*(.data*)

		. = ALIGN(4);
		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);


		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))
		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;

	} > RAM

	.bss (NOLOAD):
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > RAM

	. = ALIGN(4);
	__HeapBase = .;
	__end__ = .;
	__HeapLimit = ORIGIN(RAM) + LENGTH(RAM) - 4;

	PROVIDE(__stack = __StackTop);
	PROVIDE(__stack_limit = __StackLimit);
	PROVIDE(__heap_base = __HeapBase);
	PROVIDE(__heap_limit = __HeapLimit);
	PROVIDE(__end = __end__);
	PROVIDE(_heap = __HeapBase);
	PROVIDE(_eheap = __HeapLimit);
}

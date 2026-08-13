/* FLPR firmware linker script using Nordic's canonical FLPR SRAM partition.
 * Per Zephyr DTS (nrf54l15_cpuflpr.dtsi): FLPR's SRAM block is at M33-view
 * 0x20028000..0x20040000 (96 KB). The Zephyr vpr_launcher writes INITPC to
 * the parent (M33-bus) address of execution-memory, i.e. 0x20028000.       */

SEARCH_DIR(.)
GROUP(-lgcc)

/* ORIGIN below mirrors FLPR_EXEC_BASE in arch/cpu/nrf-vpr/shared/flpr-shared.h (a .ld
 * script cannot include the C header); keep them in sync. The 0x2003F000..
 * 0x20040000 tail of the 96 KB partition is left for the FLPR<->M33 shared
 * words (FLPR_SHARED_COUNTER_ADDR), so RAM stops at 0x2003F000. */
MEMORY
{
  FLASH (rx) : ORIGIN = 0x20028000, LENGTH = 0x8000   /* 32 KB code/.rodata */
  RAM (rwx)  : ORIGIN = 0x20030000, LENGTH = 0xF000   /* 60 KB data/.bss/stack */
}

INCLUDE "nrf_common_riscv.ld"

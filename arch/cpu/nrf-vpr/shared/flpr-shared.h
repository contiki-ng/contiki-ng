/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FLPR_SHARED_H_
#define FLPR_SHARED_H_

/*
 * Single source of truth for the FLPR <-> M33 shared memory map and status
 * markers. Included by both the FLPR firmware (TARGET=nrf-vpr) and the M33-side
 * loader (TARGET=nrf, via -I to this directory).
 *
 * NOTE: the linker script (nrf-vpr-sram.ld) hardcodes the same execution base
 * and partition size; keep the two in sync (a .ld script cannot include this
 * C header).
 */

/* FLPR execution memory: start of the 96 KB SRAM block the FLPR owns
 * (M33-bus view). The M33 copies the blob here and points INITPC at it; for an
 * SRAM-resident build the entry PC equals this base. */
#define FLPR_EXEC_BASE            0x20028000UL
#define FLPR_EXEC_SIZE            0x00018000UL   /* 96 KB partition */

/* Shared status words in the FLPR data region: the counter the FLPR advances
 * and the M33 polls, plus the slot the trap handler stores the faulting PC in. */
#define FLPR_SHARED_COUNTER_ADDR  0x2003F000UL
#define FLPR_FAULT_PC_ADDR        0x2003F004UL

/* Status values written to FLPR_SHARED_COUNTER_ADDR. */
#define FLPR_MARK_BOOT            0xA0000001UL   /* _start reached, pre-main()  */
#define FLPR_MARK_EXIT            0xA000FFFFUL   /* main() returned (shouldn't) */
#define FLPR_MARK_FAULT_BASE      0xFA110000UL   /* OR'd with mcause on a trap  */

#ifndef __ASSEMBLER__
#include <stdint.h>
#define FLPR_SHARED_COUNTER (*(volatile uint32_t *)FLPR_SHARED_COUNTER_ADDR)
#endif

#endif /* FLPR_SHARED_H_ */

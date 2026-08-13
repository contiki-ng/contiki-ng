/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* M33-side FLPR launcher — releases the FLPR (RV32E VPR coprocessor) from
 * reset with the boot sequence the nRF54L15 requires:
 *   1. memcpy blob to execution memory
 *   2. mark the VPR peripheral Secure in the SPU (SECATTR=Secure)
 *   3. set the VPR start PC (INITPC)
 *   4. set VPR CPURUN to launch
 * On this SoC the VPR is gated as a Secure peripheral, so step 2 must be done
 * from the Secure address view.
 */

#include "contiki.h"
#include "sys/log.h"
#include "sys/etimer.h"
#include "nrf.h"

#include "flpr-shared.h"
#include "flpr-blob.h"

#include <stdint.h>
#include <string.h>

/* The blob is copied to FLPR_EXEC_BASE; it must not reach the shared-counter
 * region (or it would clobber it / overrun into M33 SRAM). */
_Static_assert(sizeof(flpr_blob) <= FLPR_SHARED_COUNTER_ADDR - FLPR_EXEC_BASE,
               "FLPR blob would overrun its SRAM partition");

#define LOG_MODULE "flpr-host"
#define LOG_LEVEL  LOG_LEVEL_INFO

/* Direct register access — bypass the HAL so we know exactly what we're doing. */

#define VPR_S    ((volatile struct { uint8_t pad[0x800]; uint32_t CPURUN; uint32_t rsv; uint32_t INITPC; } *)0x5004C000UL)
#define VPR_NS   ((volatile struct { uint8_t pad[0x800]; uint32_t CPURUN; uint32_t rsv; uint32_t INITPC; } *)0x4004C000UL)

/* SPU00 PERIPH[N].PERM @ NRF_SPU00_S_BASE + 0x500 + N*4
 * VPR00 slave index = (0x4004C000 >> 12) & 0x3F = 0xC (12) */
#define SPU00_S_PERIPH_VPR00_PERM   (*(volatile uint32_t *)(0x50040000UL + 0x500UL + 12UL*4))
#define SPU00_NS_PERIPH_VPR00_PERM  (*(volatile uint32_t *)(0x40040000UL + 0x500UL + 12UL*4))

#define PERM_SECATTR_BIT   (1u << 4)

PROCESS(flpr_host_process, "flpr-host");

#ifdef FLPR_HOST_M33_LED
PROCESS(m33_led_process, "m33-led");
AUTOSTART_PROCESSES(&flpr_host_process, &m33_led_process);

/* M33-side LED1 blinker (gpio1.10 on the nRF54L15-DK) at 2 Hz toggle, distinct
 * from the FLPR's 1 Hz LED0. Enabled via FLPR_HOST_M33_LED, which the Makefile
 * sets for boards that have a second LED. The XIAO nRF54L15 has only one user
 * LED (gpio2.0, driven by the FLPR), so this is compiled out there and M33
 * liveness is observed via the serial "[FLPR] tick" logs instead. */
PROCESS_THREAD(m33_led_process, ev, data)
{
  static struct etimer led_et;
  static uint32_t toggle;

  PROCESS_BEGIN();

  NRF_P1_S->DIRSET = (1u << 10);    /* LED1 = gpio1.10 output */

  while(1) {
    etimer_set(&led_et, CLOCK_SECOND / 4);   /* 250 ms, 2 Hz blink */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&led_et));
    toggle ^= 1;
    if(toggle) {
      NRF_P1_S->OUTSET = (1u << 10);
    } else {
      NRF_P1_S->OUTCLR = (1u << 10);
    }
  }

  PROCESS_END();
}
#else
AUTOSTART_PROCESSES(&flpr_host_process);
#endif /* FLPR_HOST_M33_LED */

PROCESS_THREAD(flpr_host_process, ev, data)
{
  static struct etimer et;
  static uint32_t last_tick;

  PROCESS_BEGIN();

  LOG_INFO("M33 boot complete, blob=%u bytes\n", (unsigned)flpr_blob_len);

  /* Step 1: copy blob into FLPR execution memory. */
  memcpy((void *)FLPR_EXEC_BASE, flpr_blob, flpr_blob_len);
  LOG_INFO("Blob memcpy'd to 0x%08lx\n", (unsigned long)FLPR_EXEC_BASE);

  /* Step 2: ensure VPR is marked Secure in the SPU.
   * If our M33 is in NS, this will HardFault. If it succeeds, we're in S. */
  FLPR_SHARED_COUNTER = 0;

  uint32_t spu_perm_before;
  uint32_t spu_perm_after;
  uint32_t mode_marker;

  /* Try Secure SPU access. */
  __asm__ volatile ("" ::: "memory");
  spu_perm_before = SPU00_S_PERIPH_VPR00_PERM;
  SPU00_S_PERIPH_VPR00_PERM = spu_perm_before | PERM_SECATTR_BIT;
  spu_perm_after = SPU00_S_PERIPH_VPR00_PERM;
  mode_marker = 0xDEADC0DE;   /* if we got here, S mode works */

  LOG_INFO("SPU PERIPH[12] before=0x%08lx after=0x%08lx\n",
           (unsigned long)spu_perm_before, (unsigned long)spu_perm_after);
  LOG_INFO("M33 is in SECURE mode (SPU S access succeeded). marker=0x%08lx\n",
           (unsigned long)mode_marker);

  /* Step 3+4: set INITPC and CPURUN via the SAME view (S, matching our access). */
  VPR_S->INITPC = FLPR_EXEC_BASE;
  VPR_S->CPURUN = 1u;

  LOG_INFO("VPR_S after launch: INITPC=0x%08lx CPURUN=0x%lx\n",
           (unsigned long)VPR_S->INITPC, (unsigned long)VPR_S->CPURUN);

  etimer_set(&et, CLOCK_SECOND);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    uint32_t now = FLPR_SHARED_COUNTER;
    if(now != last_tick) {
      LOG_INFO("[FLPR] tick %u\n", (unsigned)now);
      last_tick = now;
    } else {
      uint32_t mepc = *(volatile uint32_t *)FLPR_FAULT_PC_ADDR;
      LOG_INFO("[FLPR] counter 0x%08lx mepc=0x%08lx CPURUN=0x%lx\n",
               (unsigned long)now, (unsigned long)mepc, (unsigned long)VPR_S->CPURUN);
    }
    etimer_reset(&et);
  }

  PROCESS_END();
}

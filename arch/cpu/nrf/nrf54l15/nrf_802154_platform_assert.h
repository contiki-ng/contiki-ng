/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Platform assert override for nrf_802154 on nRF54L15.
 * Prints file:line via UART and resets via NVIC_SystemReset().
 *
 * IMPORTANT: We must NOT use BKPT on the XIAO nRF54L15 because the onboard
 * CMSIS-DAP debugger keeps CoreDebug->DHCSR.C_DEBUGEN set.  When C_DEBUGEN
 * is active, BKPT halts the CPU in Debug state instead of escalating to
 * HardFault — the HardFault handler never fires, and the system hangs
 * silently until the debugger chip issues an SREQ reset.
 */
#ifndef NRF_802154_PLATFORM_ASSERT_H_
#define NRF_802154_PLATFORM_ASSERT_H_

#define NRF_802154_ASSERT(condition) do { \
    if(!(condition)) { \
      extern void uarte_write(unsigned char c); \
      uarte_write('A'); uarte_write('!'); uarte_write(' '); \
      /* Print file name */ \
      { const char *_f = __FILE__; \
        /* Skip path, just print filename */ \
        const char *_s = _f; \
        while(*_s) { if(*_s == '/') _f = _s + 1; _s++; } \
        while(*_f) uarte_write((unsigned char)*_f++); } \
      uarte_write(':'); \
      /* Print line number in decimal */ \
      { unsigned _l = __LINE__; \
        char _b[6]; int _i = 0; \
        if(_l == 0) { uarte_write('0'); } \
        else { while(_l) { _b[_i++] = '0' + (_l % 10); _l /= 10; } \
               while(_i--) uarte_write((unsigned char)_b[_i]); } } \
      uarte_write('\n'); \
      for(volatile int _d = 0; _d < 100000; _d++) {} \
      /* Reset via SCB->AIRCR (CMSIS NVIC_SystemReset equivalent) */ \
      __asm volatile("dsb 0xF":::"memory"); \
      *((volatile unsigned long *)0xE000ED0CUL) = 0x05FA0004UL; \
      __asm volatile("dsb 0xF":::"memory"); \
      for(;;) { __asm volatile("nop"); } \
    } \
  } while(0)

#endif /* NRF_802154_PLATFORM_ASSERT_H_ */

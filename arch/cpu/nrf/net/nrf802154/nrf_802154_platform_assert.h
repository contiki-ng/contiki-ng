/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/**
 * \file
 *      Platform assert override for nrf_802154 on the nRF5340 network core.
 *      Adapted from arch/cpu/nrf/nrf54l15/nrf_802154_platform_assert.h.
 *
 *      The network core has no UARTE (NRF_HAS_UARTE 0); its debug output is
 *      forwarded to the application core over the IPC log ring buffer via
 *      dbg_putchar(). On an internal library assertion this reports
 *      "802154! file:line" through that path and then resets the core. BKPT
 *      is avoided on purpose: with a debugger attached it would halt in
 *      Debug state instead of escalating, hanging the core silently.
 */
/*---------------------------------------------------------------------------*/
#ifndef NRF_802154_PLATFORM_ASSERT_H_
#define NRF_802154_PLATFORM_ASSERT_H_
/*---------------------------------------------------------------------------*/
__attribute__((noreturn))
static inline void
nrf_802154_platform_assert_fail(const char *file, unsigned line)
{
  extern int dbg_putchar(int c);
  const char *basename = file;
  const char *p;
  char digits[10];
  int i;
  volatile int d;

  for(p = "802154! "; *p != '\0'; p++) {
    dbg_putchar(*p);
  }

  /* Print the basename of __FILE__. */
  for(p = file; *p != '\0'; p++) {
    if(*p == '/') {
      basename = p + 1;
    }
  }
  for(p = basename; *p != '\0'; p++) {
    dbg_putchar(*p);
  }
  dbg_putchar(':');

  /* Print __LINE__ in decimal. */
  if(line == 0) {
    dbg_putchar('0');
  } else {
    i = 0;
    while(line > 0) {
      digits[i++] = (char)('0' + (line % 10));
      line /= 10;
    }
    while(i-- > 0) {
      dbg_putchar(digits[i]);
    }
  }
  dbg_putchar('\n');

  /* Brief spin so the IPC log drains before reset. */
  for(d = 0; d < 100000; d++) {
  }

  /* System reset via SCB->AIRCR (CMSIS NVIC_SystemReset() equivalent). */
  __asm volatile("dsb 0xF" ::: "memory");
  *((volatile unsigned long *)0xE000ED0CUL) = 0x05FA0004UL;
  __asm volatile("dsb 0xF" ::: "memory");
  for(;;) {
    __asm volatile("nop");
  }
}
/*---------------------------------------------------------------------------*/
#define NRF_802154_ASSERT(condition)                                    \
  do {                                                                  \
    if(!(condition)) {                                                  \
      nrf_802154_platform_assert_fail(__FILE__, __LINE__);              \
    }                                                                   \
  } while(0)
/*---------------------------------------------------------------------------*/
#endif /* NRF_802154_PLATFORM_ASSERT_H_ */

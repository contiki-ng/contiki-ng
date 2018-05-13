/*
 * Copyright (c) 2007, Swedish Institute of Computer Science
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef MSP430_DEF_H_
#define MSP430_DEF_H_

#ifdef __IAR_SYSTEMS_ICC__
#include <intrinsics.h>
#include <in430.h>
#include <msp430.h>
#define dint() __disable_interrupt()
#define eint() __enable_interrupt()
#define __MSP430__ 1
#define CC_CONF_INLINE

#else /* __IAR_SYSTEMS_ICC__ */

#ifdef __MSPGCC__
#include <msp430.h>
#include <legacymsp430.h>
#else /* __MSPGCC__ */
#include <io.h>
#include <signal.h>
#if !defined(MSP430_MEMCPY_WORKAROUND) && (__GNUC__ < 4)
#define MSP430_MEMCPY_WORKAROUND 1
#endif
#endif /* __MSPGCC__ */

#define CC_CONF_INLINE inline

#endif /* __IAR_SYSTEMS_ICC__ */

/* Master interrupt state representation data type */
#define INT_MASTER_CONF_STATUS_DATATYPE __istate_t

#ifndef BV
#define BV(x) (1 << x)
#endif

#include <stdint.h>

/* These names are deprecated, use C99 names. */
typedef  uint8_t    u8_t;
typedef uint16_t   u16_t;
typedef uint32_t   u32_t;
typedef  int32_t   s32_t;

/* Types for clocks and uip_stats */
typedef unsigned short uip_stats_t;
typedef unsigned long clock_time_t;
typedef long off_t;

/* Our clock resolution, this is the same as Unix HZ. */
#define CLOCK_CONF_SECOND 128UL

/* Use 16-bit rtimer (default in Contiki-NG is 32) */
#define RTIMER_CONF_CLOCK_SIZE 2

typedef int spl_t;
spl_t   splhigh_(void);

#define splhigh() splhigh_()
#ifdef __IAR_SYSTEMS_ICC__
#define splx(sr) __bis_SR_register(sr)
#else
#define splx(sr) __asm__ __volatile__("bis %0, r2" : : "r" (sr))
#endif

/* Workaround for bug in msp430-gcc compiler */
#if defined(__MSP430__) && defined(__GNUC__) && MSP430_MEMCPY_WORKAROUND
#ifndef memcpy
#include <string.h>

void *w_memcpy(void *out, const void *in, size_t n);
#define memcpy(dest, src, count) w_memcpy(dest, src, count)

void *w_memset(void *out, int value, size_t n);
#define memset(dest, value, count) w_memset(dest, value, count)

#endif /* memcpy */
#endif /* __GNUC__ &&  __MSP430__ && MSP430_MEMCPY_WORKAROUND */

#define memory_barrier()   asm volatile("" : : : "memory")

#define MSP430_REQUIRE_CPUON 0
#define MSP430_REQUIRE_LPM1 1
#define MSP430_REQUIRE_LPM2 2
#define MSP430_REQUIRE_LPM3 3

/* Platform-specific checksum implementation */
#define UIP_ARCH_IPCHKSUM        1

#define BAUD2UBR(baud) ((F_CPU/baud))

void msp430_add_lpm_req(int req);
void msp430_remove_lpm_req(int req);
void msp430_cpu_init(void); /* Rename to cpu_init() later! */
void msp430_sync_dco(void);
#define cpu_init() msp430_cpu_init()
void   *sbrk(int);

#endif /* MSP430_DEF_H_ */

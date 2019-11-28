/*
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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

#ifndef EFR32_DEF_H_
#define EFR32_DEF_H_
/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/**
 * \name Macros and typedefs
 *
 * Those values are not meant to be modified by the user
 * @{
 */
#define RTIMER_CONF_CLOCK_SIZE 2
/* 38.4 MHz divided by 1024 */
#define RTIMER_ARCH_SECOND  (38400000UL / 1024)

#define CLOCK_CONF_SECOND 1000

/* Clock (time) comparison macro */
#define CLOCK_LT(a, b)  ((int64_t)((a) - (b)) < 0)

/* Platform typedefs */
typedef uint64_t clock_time_t;
typedef uint32_t uip_stats_t;
/*---------------------------------------------------------------------------*/
/* Path to CMSIS header */
#define CMSIS_CONF_HEADER_PATH               "efr32-cm4.h"
/* Path to headers with implementation of mutexes and memory barriers */
#define MUTEX_CONF_ARCH_HEADER_PATH          "mutex-cortex.h"
#define MEMORY_BARRIER_CONF_ARCH_HEADER_PATH "memory-barrier-cortex.h"
/*---------------------------------------------------------------------------*/
#define I2C_HAL_CONF_ARCH_HDR_PATH          "dev/i2c-hal-arch.h"
/*---------------------------------------------------------------------------*/
#define GPIO_HAL_CONF_ARCH_HDR_PATH         "em_gpio.h"
#define GPIO_HAL_CONF_ARCH_SW_TOGGLE 0
/*---------------------------------------------------------------------------*/
/*
 * The stdio.h that ships with the arm-gcc toolchain does this:
 *
 * int  _EXFUN(putchar, (int));
 * [...]
 * #define  putchar(x)  putc(x, stdout)
 *
 * This causes us a lot of trouble: For platforms using this toolchain, every
 * time we use putchar we need to first #undef putchar. What we do here is to
 * #undef putchar across the board. The resulting code will cause the linker
 * to search for a symbol named putchar and this allows us to use the
 * implementation under os/lib/dbg-io.
 *
 * This will fail if stdio.h is included before contiki.h, but it is common
 * practice to include contiki.h first
 */
#include <stdio.h>
#undef putchar

#endif /* EFR32_DEF_H_ */

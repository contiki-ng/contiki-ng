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
/*
 * Force-included into the Core SDK library build for the CC13x2x7/CC26x2x7:
 * maps driverlib names the Core SDK's TI Drivers (SDK 4.10) use onto their
 * CC13x2x7/CC26x2x7 driverlib (SDK 7.x) equivalents. Each mapping is a
 * rename with an identical implementation, or, for a helper SDK 7.x dropped,
 * the SDK 4.10 driverlib's own definition.
 */
#ifndef CORESDK_COMPAT_H_
#define CORESDK_COMPAT_H_

/* prcm.c: PRCMPowerDomainStatus() became PRCMPowerDomainsAllOn(). */
#define PRCMPowerDomainStatus PRCMPowerDomainsAllOn

/* gpio.h: GPIO_readMultiDio() and GPIO_writeMultiDio() are gone from SDK 7.x
 * (GPIOCC26XX.c uses them). As in the SDK 4.10 driverlib. */
#include <stdint.h>
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_types.h)
#include DeviceFamily_constructPath(inc/hw_memmap.h)
#include DeviceFamily_constructPath(inc/hw_gpio.h)

static inline uint32_t
GPIO_readMultiDio(uint32_t dioMask)
{
  return HWREG(GPIO_BASE + GPIO_O_DIN31_0) & dioMask;
}

static inline void
GPIO_writeMultiDio(uint32_t dioMask, uint32_t bitVectoredValue)
{
  HWREG(GPIO_BASE + GPIO_O_DOUT31_0) =
    (HWREG(GPIO_BASE + GPIO_O_DOUT31_0) & ~dioMask) |
    (bitVectoredValue & dioMask);
}

#endif /* CORESDK_COMPAT_H_ */

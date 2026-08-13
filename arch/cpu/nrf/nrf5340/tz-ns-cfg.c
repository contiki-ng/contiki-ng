/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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

/**
 * \file
 *   Normal-world nRF5340 hooks for the TrustZone API.
 *
 *   Provides the NS-side wake mechanism that the secure side pends
 *   via tz_arch_signal_ns. EGU0_IRQn is borrowed as a software-only
 *   doorbell: the EGU peripheral itself is unused, only its NVIC slot.
 */

#include "contiki.h"
#include "trustzone/normal/tz-normal.h"

#include <nrfx.h>

/*---------------------------------------------------------------------------*/
void
tz_arch_init_ns_signal(void)
{
  /*
   * Do not clear pending before enabling: the only source of an EGU0
   * pending bit is a deliberate tz_arch_signal_ns() from secure code,
   * so clearing here would silently discard a legitimate wake request.
   */
  NVIC_EnableIRQ(EGU0_IRQn);
}
/*---------------------------------------------------------------------------*/
void
EGU0_IRQHandler(void)
{
  tz_normal_request_poll();
}
/*---------------------------------------------------------------------------*/

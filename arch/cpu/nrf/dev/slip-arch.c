/*
 * Copyright (C) 2020-2021 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/slip.h"
#include "uarte-arch.h"
#include "usb.h"
/*---------------------------------------------------------------------------*/
#ifdef PLAFTORM_SLIP_ARCH_CONF_USB
#error Please use PLATFORM_SLIP_ARCH_CONF_USB instead of PLAFTORM_SLIP_ARCH_CONF_USB
#endif
/*---------------------------------------------------------------------------*/
#if PLATFORM_SLIP_ARCH_CONF_USB
#define set_input(fn) usb_set_input(fn)
#define write_byte(b) usb_write((uint8_t *)&b, sizeof(uint8_t))
#define flush()       usb_flush()
#else /* PLATFORM_DBG_CONF_USB */
#define set_input(fn) uarte_set_input(fn)
#define write_byte(b) uarte_write(b)
#define flush()
#endif /* PLATFORM_DBG_CONF_USB */
#define SLIP_END     0300
/*---------------------------------------------------------------------------*/
void
slip_arch_writeb(unsigned char c)
{
  write_byte(c);
  if(c == SLIP_END) {
    flush();
  }
}
/*---------------------------------------------------------------------------*/
void
slip_arch_init()
{
  set_input(slip_input_byte);
}
/*---------------------------------------------------------------------------*/

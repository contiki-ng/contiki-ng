/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
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
#include "nrf52840-conf.h"
#include "dev/slip.h"
#include "dev/uart0.h"
#include "usb/usb-serial.h"
/*---------------------------------------------------------------------------*/
#ifndef SLIP_ARCH_CONF_USB
#define SLIP_ARCH_CONF_USB 0
#endif

#if SLIP_ARCH_CONF_USB
#define write_byte(b) usb_serial_writeb(b)
#define set_input(f)  usb_serial_set_input(f)
#define flush()       usb_serial_flush()
#else
#define write_byte(b) uart0_writeb(b)
#define set_input(f)  uart0_set_input(f)
#define flush()
#endif

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

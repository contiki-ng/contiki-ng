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
 * \addtogroup cc26xx-uart
 * @{
 *
 * \file
 * Implementation of the CC13xx/CC26xx UART driver.
 */
/*---------------------------------------------------------------------------*/
#include <Board.h>
#include <ti/drivers/UART.h>

#include <stdint.h>

#include "uart0-arch.h"

static UART_Handle gh_uart;

static volatile uart0_input_cb g_input_cb;
static unsigned char g_char_buf;

/*---------------------------------------------------------------------------*/
static void
uart0_cb(UART_Handle handle, void *buf, size_t count)
{
  if (!g_input_cb) { return; }

  const uart0_input_cb currCb = g_input_cb;
  currCb(g_char_buf);
  if (currCb == g_input_cb)
  {
    UART_read(gh_uart, &g_char_buf, 1);
  }
}
/*---------------------------------------------------------------------------*/
void
uart0_init(void)
{
  UART_init();

  UART_Params params;
  UART_Params_init(&params);
  params.readMode = UART_MODE_CALLBACK;
  params.writeMode = UART_MODE_BLOCKING;
  params.readCallback = uart0_cb;
  params.readDataMode = UART_DATA_TEXT;
  params.readReturnMode = UART_RETURN_NEWLINE;

  gh_uart = UART_open(Board_UART0, &params);
  if (!gh_uart)
  {
    for (;;) { /* hang */ }
  }
}
/*---------------------------------------------------------------------------*/
int_fast32_t
uart0_write(const void *buffer, size_t size)
{
  if (!gh_uart)
  {
    return UART_STATUS_ERROR;
  }
  return UART_write(gh_uart, buffer, size);
}
/*---------------------------------------------------------------------------*/
void
uart0_set_callback(uart0_input_cb input_cb)
{
  if (g_input_cb == input_cb) { return; }

  g_input_cb = input_cb;
  if (input_cb)
  {
    UART_read(gh_uart, &g_char_buf, 1);
  }
  else
  {
    UART_readCancel(gh_uart);
  }
}
/*---------------------------------------------------------------------------*/
/** @} */

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
/*---------------------------------------------------------------------------*/
#include <contiki.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#include "uart0-arch.h"
/*---------------------------------------------------------------------------*/
static UART_Handle gh_uart;

static volatile uart0_input_cb curr_input_cb;
static unsigned char char_buf;

static bool is_init;
/*---------------------------------------------------------------------------*/
static void
uart0_cb(UART_Handle handle, void *buf, size_t count)
{
  if (!curr_input_cb) { return; }

  /*
   * Save the current callback function, as this might be overwritten after
   * the callback is called.
   */
  const uart0_input_cb currCb = curr_input_cb;
  /* Call the callback. Note this might reset curr_input_cb */
  currCb(char_buf);
  /*
   * If curr_input_cb didn't change after the call, do another read.
   * Else, the uart0_set_callback was called with a different callback pointer
   * and triggered an another read.
   */
  if (currCb == curr_input_cb) {
    UART_read(gh_uart, &char_buf, 1);
  }
}
/*---------------------------------------------------------------------------*/
void
uart0_init(void)
{
  if (is_init) { return; }
  is_init = true;

  UART_Params params;
  UART_Params_init(&params);

  params.baudRate       = TI_UART_CONF_BAUD_RATE;
  params.readMode       = UART_MODE_CALLBACK;
  params.writeMode      = UART_MODE_BLOCKING;
  params.readCallback   = uart0_cb;
  params.readDataMode   = UART_DATA_TEXT;
  params.readReturnMode = UART_RETURN_NEWLINE;

  gh_uart = UART_open(Board_UART0, &params);
}
/*---------------------------------------------------------------------------*/
int_fast32_t
uart0_write(const void *buffer, size_t size)
{
  if (!is_init) {
    return UART_STATUS_ERROR;
  }
  return UART_write(gh_uart, buffer, size);
}
/*---------------------------------------------------------------------------*/
int_fast32_t
uart0_set_callback(uart0_input_cb input_cb)
{
  if (!is_init) {
    return UART_STATUS_ERROR;
  }

  if (curr_input_cb == input_cb) {
    return UART_STATUS_SUCCESS;
  }

  curr_input_cb = input_cb;
  if (input_cb) {
    return UART_read(gh_uart, &char_buf, 1);
  } else {
    UART_readCancel(gh_uart);
    return UART_STATUS_SUCCESS;
  }
}
/*---------------------------------------------------------------------------*/
/** @} */

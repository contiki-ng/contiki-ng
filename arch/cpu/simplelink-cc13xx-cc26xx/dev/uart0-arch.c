/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
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
/**
 * \addtogroup cc13xx-cc26xx-uart
 * @{
 *
 * \file
 *        Implementation of UART driver for CC13xx/CC26xx.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include "uart0-arch.h"
/*---------------------------------------------------------------------------*/
#include <Board.h>

#include <ti/drivers/UART.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
static UART_Handle uart_handle;

static volatile uart0_input_fxn_t curr_input_cb;
static unsigned char char_buf;

static bool initialized;
/*---------------------------------------------------------------------------*/
static void
uart0_cb(UART_Handle handle, void *buf, size_t count)
{
  /* Simply return if the current callback is NULL. */
  if(!curr_input_cb) {
    return;
  }

  /*
   * Save the current callback function locally, as it might be overwritten
   * after calling the callback.
   */
  const uart0_input_fxn_t curr_cb = curr_input_cb;
  curr_cb(char_buf);
  /*
   * If curr_input_cb didn't change after the call, do another read.
   * Else, the uart0_set_callback was called with a different callback pointer
   * and triggered an another read.
   */
  if(curr_cb == curr_input_cb) {
    UART_read(uart_handle, &char_buf, 1);
  }
}
/*---------------------------------------------------------------------------*/
void
uart0_init(void)
{
  if(initialized) {
    return;
  }

  UART_Params uart_params;
  UART_Params_init(&uart_params);

  uart_params.baudRate = TI_UART_CONF_BAUD_RATE;
  uart_params.readMode = UART_MODE_CALLBACK;
  uart_params.writeMode = UART_MODE_BLOCKING;
  uart_params.readCallback = uart0_cb;
  uart_params.readDataMode = UART_DATA_TEXT;
  uart_params.readReturnMode = UART_RETURN_NEWLINE;

  /* No error handling. */
  uart_handle = UART_open(Board_UART0, &uart_params);

  initialized = true;
}
/*---------------------------------------------------------------------------*/
int_fast32_t
uart0_write(const void *buf, size_t buf_size)
{
  if(!initialized) {
    return UART_STATUS_ERROR;
  }
  return UART_write(uart_handle, buf, buf_size);
}
/*---------------------------------------------------------------------------*/
int_fast32_t
uart0_write_byte(uint8_t byte)
{
  if(!initialized) {
    return UART_STATUS_ERROR;
  }
  return UART_write(uart_handle, &byte, 1);
}
/*---------------------------------------------------------------------------*/
int_fast32_t
uart0_set_callback(uart0_input_fxn_t input_cb)
{
  if(!initialized) {
    return UART_STATUS_ERROR;
  }

  if(curr_input_cb == input_cb) {
    return UART_STATUS_SUCCESS;
  }

  curr_input_cb = input_cb;
  if(input_cb) {
    return UART_read(uart_handle, &char_buf, 1);
  } else {
    UART_readCancel(uart_handle);
    return UART_STATUS_SUCCESS;
  }
}
/*---------------------------------------------------------------------------*/
/** @} */

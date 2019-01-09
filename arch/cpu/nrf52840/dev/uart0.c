/*
 * Copyright (c) 2015, Nordic Semiconductor
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
 *
 */
/**
 * \addtogroup nrf52840-dev Device drivers
 * @{
 *
 * \addtogroup nrf52840-uart UART driver
 * @{
 *
 * \file
 *         Contiki compatible UART driver.
 * \author
 *         Wojciech Bober <wojciech.bober@nordicsemi.no>
 */
#include <stdlib.h>
#include "nrf.h"
#include "sdk_config.h"
#include "nrfx_uart.h"
#include "app_util_platform.h"
#include "app_error.h"

#include "contiki.h"
#include "dev/uart0.h"
#include "dev/watchdog.h"
#include "lib/ringbuf.h"

#define TXBUFSIZE 128
static uint8_t rx_buffer[1];

static int (*uart0_input_handler)(unsigned char c);

static struct ringbuf txbuf;
static uint8_t txbuf_data[TXBUFSIZE];

static nrfx_uart_t uart_inst = NRFX_UART_INSTANCE(0);

/*---------------------------------------------------------------------------*/
static void
uart_event_handler(const nrfx_uart_event_t * p_event, void * p_context)
{
  if (p_event->type == NRFX_UART_EVT_RX_DONE) {
    if (uart0_input_handler != NULL) {
      uart0_input_handler(p_event->data.rxtx.p_data[0]);
    }
    (void)nrfx_uart_rx(&uart_inst, rx_buffer, 1);
  } else if (p_event->type == NRFX_UART_EVT_TX_DONE) {
    if (ringbuf_elements(&txbuf) > 0) {
      uint8_t c = ringbuf_get(&txbuf);
      nrfx_uart_tx(&uart_inst, &c, 1);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
uart0_set_input(int (*input)(unsigned char c))
{
  uart0_input_handler = input;
}
/*---------------------------------------------------------------------------*/
void
uart0_writeb(unsigned char c)
{
  if (nrfx_uart_tx(&uart_inst, &c, 1) == NRF_ERROR_BUSY) {
    while (ringbuf_put(&txbuf, c) == 0) {
      __WFE();
    }
  }
}
/*---------------------------------------------------------------------------*/
/**
 * Initialize the RS232 port.
 *
 */
void
uart0_init(unsigned long ubr)
{
  nrfx_uart_config_t config = NRFX_UART_DEFAULT_CONFIG;
  config.pseltxd = 6;
  config.pselrxd = 8;
  config.pselcts = 7;
  config.pselrts = 5;
  ret_code_t retcode = nrfx_uart_init(&uart_inst, &config, uart_event_handler);
  APP_ERROR_CHECK(retcode);
  if(retcode != NRFX_SUCCESS) {
    perror("nrfx_uart_init()");
  }

  ringbuf_init(&txbuf, txbuf_data, sizeof(txbuf_data));

  /* nrfx_uart_rx_enable(&uart_inst); */
  /* nrfx_uart_rx(&uart_inst, rx_buffer, 1); */
}
/**
 * @}
 * @}
 */

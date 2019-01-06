/*
 * Copyright (c) 2018, Yanzi Networks
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
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
#include <em_device.h>
#include <em_usart.h>
#include <em_cmu.h>
#include <em_gpio.h>
#include "lib/dbg-io/dbg.h"
#include "debug-uart.h"
#include <stdbool.h>
#include <stdio.h>

#ifndef DEBUG_UART_TX_LOC
#error "debug-uart: no UART TX_LOC configured"
#endif /* DEBUG_UART_TX_LOC */

#define TX_LOC        DEBUG_UART_TX_LOC
#define TX_PORT       AF_USART0_TX_PORT(TX_LOC)
#define TX_PIN        AF_USART0_TX_PIN(TX_LOC)

#ifdef DEBUG_UART_RX_LOC
#define RX_LOC        DEBUG_UART_RX_LOC
#define RX_PORT       AF_USART0_RX_PORT(RX_LOC)
#define RX_PIN        AF_USART0_RX_PIN(RX_LOC)
#endif /* DEBUG_UART_RX_LOC */

#ifdef DEBUG_UART_CONF_TX_BUFFER
#define TX_SIZE DEBUG_UART_CONF_TX_BUFFER
#else /* DEBUG_UART_CONF_TX_BUFFER */
#define TX_SIZE       128
#endif /* DEBUG_UART_CONF_TX_BUFFER */

#define HANDLED_RX_ERR (USART_IF_FERR | USART_IF_PERR | USART_IF_RXOF)

static uint8_t TXBUFFER[TX_SIZE] = {0};
/* first char to read */
static volatile uint16_t rpos = 0;
/* last char written (or next to write) */
static volatile uint16_t wpos = 0;

void USART0_RX_IRQHandler() __attribute__((interrupt));
void USART0_TX_IRQHandler() __attribute__((interrupt));

static USART_TypeDef * g_uart = USART0;
static int (* input_handler)(unsigned char c);
/*---------------------------------------------------------------------------*/
static void
send_txbuf(void)
{
  while(rpos != wpos && (g_uart->STATUS & USART_STATUS_TXBL)) {
    g_uart->TXDATA = (uint32_t)TXBUFFER[rpos];
    rpos = ((rpos + 1) % TX_SIZE);
  }
}
/*---------------------------------------------------------------------------*/
static inline void
write_txbuf(char c)
{
  TXBUFFER[wpos] = c;
  wpos = ((wpos + 1) % TX_SIZE);
}
/*---------------------------------------------------------------------------*/
static inline bool
is_tx_buffer_full(void)
{
  return wpos + 1 == rpos || (wpos + 1 == TX_SIZE && rpos == 0);
}
/*---------------------------------------------------------------------------*/
static inline bool
clear_full_fifo(void)
{
  if(is_tx_buffer_full()) {
    rpos = ((rpos + 1) % TX_SIZE);
    return true;
  } else {
    return false;
  }
}
/*---------------------------------------------------------------------------*/
static inline void
write_byte(char c)
{
  static bool dropping = false;
  if(is_tx_buffer_full()) {
    if(!dropping) {
      /* Wait for a short time to allow current data byte to finish transfer */
      rtimer_clock_t t0 = RTIMER_NOW();
      while(is_tx_buffer_full() &&
            RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + RTIMER_SECOND / 1000)) {
      }
    }
  }

  NVIC_DisableIRQ(USART0_TX_IRQn);
  dropping = clear_full_fifo();
  write_txbuf(c);
  send_txbuf();
  if (rpos != wpos) {
    NVIC_ClearPendingIRQ(USART0_TX_IRQn);
    NVIC_EnableIRQ(USART0_TX_IRQn);
  }
}
/*---------------------------------------------------------------------------*/
void
USART0_TX_IRQHandler(void)
{
  send_txbuf();
  if(rpos != wpos) {
    NVIC_ClearPendingIRQ(USART0_TX_IRQn);
  } else {
    NVIC_DisableIRQ(USART0_TX_IRQn);
  }
}
/*---------------------------------------------------------------------------*/
void
USART0_RX_IRQHandler(void)
{
  uint32_t flags = USART_IntGetEnabled(g_uart);

  USART_IntClear(g_uart, flags & HANDLED_RX_ERR);

  if(flags & USART_IF_RXDATAV) {
    unsigned char c;
    while(g_uart->STATUS & USART_STATUS_RXDATAV) {
      c = USART_RxDataGet(g_uart);
      if(input_handler) {
        input_handler(c);
      }
    }
  }

  NVIC_ClearPendingIRQ(USART0_RX_IRQn);
}
/*---------------------------------------------------------------------------*/
int
debug_uart_is_busy(void)
{
  return rpos != wpos;
}
/*---------------------------------------------------------------------------*/
void
debug_uart_init(void)
{
  USART_InitAsync_TypeDef init  = USART_INITASYNC_DEFAULT;

#ifdef SERIAL_BAUDRATE
  init.baudrate = SERIAL_BAUDRATE;
#endif /* SERIAL_BAUDRATE */

  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_USART0, true);

  GPIO_PinModeSet(TX_PORT, TX_PIN, gpioModePushPull, 1);

#ifdef DEBUG_UART_RX_LOC
  GPIO_PinModeSet(RX_PORT, RX_PIN, gpioModeInput, 0);
#endif /* DEBUG_UART_RX_LOC */

  init.enable = usartDisable;

  USART_InitAsync(g_uart, &init);

  /* Raise TXBL as soon as there is at least one empty slot */
  g_uart->CTRL |= USART_CTRL_TXBIL;

#ifdef DEBUG_UART_RX_LOC
  g_uart->ROUTELOC0 = (g_uart->ROUTELOC0 & ~(_USART_ROUTELOC0_TXLOC_MASK
                                            | _USART_ROUTELOC0_RXLOC_MASK ))
                       | (TX_LOC << _USART_ROUTELOC0_TXLOC_SHIFT)
                       | (RX_LOC << _USART_ROUTELOC0_RXLOC_SHIFT);
  g_uart->ROUTEPEN |= USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
#else /* DEBUG_UART_RX_LOC */
  g_uart->ROUTELOC0 = (g_uart->ROUTELOC0 & ~(_USART_ROUTELOC0_TXLOC_MASK
                                            | _USART_ROUTELOC0_RXLOC_MASK ))
                       | (TX_LOC << _USART_ROUTELOC0_TXLOC_SHIFT);
  g_uart->ROUTEPEN |= USART_ROUTEPEN_TXPEN;
#endif /* DEBUG_UART_RX_LOC */

  USART_Enable(g_uart, usartEnable);

  USART_IntEnable(g_uart, USART_IF_TXBL);
  /* USART0_TX_IRQn will be enabled in NVIC when there is data to send */
}
/*---------------------------------------------------------------------------*/
void
debug_uart_set_input_handler(int (* handler)(unsigned char c))
{
  input_handler = handler;

#ifdef DEBUG_UART_RX_LOC
  USART_IntClear(g_uart, HANDLED_RX_ERR);
  NVIC_ClearPendingIRQ(USART0_RX_IRQn);

  USART_IntEnable(g_uart, USART_IF_RXDATAV | HANDLED_RX_ERR);
  NVIC_EnableIRQ(USART0_RX_IRQn);
#endif /* DEBUG_UART_RX_LOC */
}
/*---------------------------------------------------------------------------*/
unsigned int
dbg_send_bytes(const unsigned char *seq, unsigned int len)
{
  for(int i=0; i < len; i++) {
    dbg_putchar(seq[i]);
  }

  return len;
}
/*---------------------------------------------------------------------------*/
int
dbg_putchar(int ch)
{
  write_byte(ch);
  return ch;
}

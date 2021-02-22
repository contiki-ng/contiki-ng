/*
 * Copyright (c) 2020, Alex Stanoev - https://astanoev.com
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
/**
 * \addtogroup nrf52840-usb
 * @{
 *
 * \file
 * Wrapper around the nRF SDK USB CDC-ACM implementation.
 * Implements a UART-like functionality over the nRF52840's USB hardware.
 *
 * This exposes a similar interface to cpu/cc2538/usb
 *
 * \author
 *  - Alex Stanoev - <alex@astanoev.com>
 */
#include "contiki.h"

#include "nrf_drv_power.h"
#include "nrf_drv_usbd.h"
#include "app_usbd.h"
#include "app_usbd_core.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
static void
cdc_acm_port_ev_handler(app_usbd_class_inst_t const *p_inst,
                        app_usbd_cdc_acm_user_event_t event);
/*---------------------------------------------------------------------------*/
#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1

APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm, cdc_acm_port_ev_handler,
                            CDC_ACM_COMM_INTERFACE, CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN, CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_NONE);
/*---------------------------------------------------------------------------*/
#define RX_BUFFER_SIZE NRF_DRV_USBD_EPSIZE
#define TX_BUFFER_SIZE NRF_DRV_USBD_EPSIZE

static uint8_t usb_rx_data[RX_BUFFER_SIZE];
static uint8_t usb_tx_data[TX_BUFFER_SIZE];

static volatile uint8_t enabled = 0;
static volatile uint8_t buffered_data = 0;
static volatile uint8_t tx_buffer_busy = 0;
/*---------------------------------------------------------------------------*/
/* Callback to the input handler */
static int (*input_handler)(unsigned char c);
/*---------------------------------------------------------------------------*/
static void
cdc_acm_port_ev_handler(app_usbd_class_inst_t const *p_inst,
                        app_usbd_cdc_acm_user_event_t event)
{
  app_usbd_cdc_acm_t const *p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);

  switch(event) {
  case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
  {
    /* Set up first transfer */
    app_usbd_cdc_acm_read_any(&m_app_cdc_acm, usb_rx_data, RX_BUFFER_SIZE);
    enabled = 1;
    break;
  }

  case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
  {
    tx_buffer_busy = 0;
    enabled = 0;
    break;
  }

  case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
  {
    tx_buffer_busy = 0;
    break;
  }

  case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
  {
    ret_code_t ret;

    do {
      size_t rx_size = app_usbd_cdc_acm_rx_size(p_cdc_acm);

      if(input_handler) {
        uint8_t i;

        for(i = 0; i < rx_size; i++) {
          input_handler(usb_rx_data[i]);
        }
      }

      /* Fetch up to RX_BUFFER_SIZE bytes from the internal buffer */
      ret = app_usbd_cdc_acm_read_any(&m_app_cdc_acm,
                                      usb_rx_data, RX_BUFFER_SIZE);
    } while(ret == NRF_SUCCESS);

    break;
  }

  default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
static void
usbd_user_ev_handler(app_usbd_event_type_t event)
{
  switch(event) {
  case APP_USBD_EVT_STOPPED:
  {
    tx_buffer_busy = 0;
    enabled = 0;
    app_usbd_disable();
    break;
  }

  case APP_USBD_EVT_POWER_DETECTED:
  {
    if(!nrf_drv_usbd_is_enabled()) {
      app_usbd_enable();
    }
    break;
  }

  case APP_USBD_EVT_POWER_REMOVED:
  {
    tx_buffer_busy = 0;
    enabled = 0;
    app_usbd_stop();
    break;
  }

  case APP_USBD_EVT_POWER_READY:
  {
    app_usbd_start();
    break;
  }

  default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
void
usb_serial_init(void)
{
  static const app_usbd_config_t usbd_config = {
    .ev_state_proc = usbd_user_ev_handler
  };

  ret_code_t ret;
  app_usbd_class_inst_t const *class_cdc_acm;

  app_usbd_serial_num_generate();

  ret = app_usbd_init(&usbd_config);
  if(ret != NRF_SUCCESS) {
    return;
  }

  class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);

  ret = app_usbd_class_append(class_cdc_acm);
  if(ret != NRF_SUCCESS) {
    return;
  }

  ret = app_usbd_power_events_enable();
  if(ret != NRF_SUCCESS) {
    return;
  }

  enabled = 0;
  buffered_data = 0;
}
/*---------------------------------------------------------------------------*/
void
usb_serial_flush()
{
  if(!enabled || tx_buffer_busy || buffered_data == 0) {
    return;
  }

  ret_code_t ret;

  tx_buffer_busy = 1;
  do {
    ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, usb_tx_data, buffered_data);
    watchdog_periodic();
  } while(ret != NRF_SUCCESS);

  /* Block until APP_USBD_CDC_ACM_USER_EVT_TX_DONE fires */
  while(tx_buffer_busy) {
    watchdog_periodic();
  }

  buffered_data = 0;
}
/*---------------------------------------------------------------------------*/
void
usb_serial_writeb(uint8_t b)
{
  if(!enabled) {
    buffered_data = 0;
    return;
  }

  if(buffered_data < TX_BUFFER_SIZE) {
    usb_tx_data[buffered_data] = b;
    buffered_data++;
  }

  if(buffered_data == TX_BUFFER_SIZE) {
    usb_serial_flush();
  }
}
/*---------------------------------------------------------------------------*/
void
usb_serial_set_input(int (*input)(unsigned char c))
{
  input_handler = input;
}
/*---------------------------------------------------------------------------*/

/** @} */

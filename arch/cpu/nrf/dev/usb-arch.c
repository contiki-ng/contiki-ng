/*
 * Copyright (C) 2021 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-dev Device drivers
 * @{
 *
 * \addtogroup nrf-usb USB driver
 * @{
 *
 * \file
 *         USB implementation for the nRF.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#if NRF_HAS_USB
/*---------------------------------------------------------------------------*/
#include "usb.h"

#include "nrfx.h"
#include "nrfx_power.h"
/*---------------------------------------------------------------------------*/
/* extern since it is not available in any header file in the tinyusb */
extern void tusb_hal_nrf_power_event(uint32_t event);
/*---------------------------------------------------------------------------*/
void
USBD_IRQHandler(void)
{
  usb_interrupt_handler();
}
/*---------------------------------------------------------------------------*/
static void
power_event_handler(nrfx_power_usb_evt_t event)
{
  tusb_hal_nrf_power_event((uint32_t)event);
}
/*---------------------------------------------------------------------------*/
void
usb_arch_init(void)
{
  const nrfx_power_config_t power_config = { 0 };
  const nrfx_power_usbevt_config_t power_usbevt_config = {
    .handler = power_event_handler
  };

  nrfx_power_init(&power_config);

  nrfx_power_usbevt_init(&power_usbevt_config);

  nrfx_power_usbevt_enable();

  nrfx_power_usb_state_t usb_reg = nrfx_power_usbstatus_get();
  if(usb_reg == NRFX_POWER_USB_STATE_CONNECTED) {
    tusb_hal_nrf_power_event(NRFX_POWER_USB_EVT_DETECTED);
  } else if(usb_reg == NRFX_POWER_USB_STATE_READY) {
    tusb_hal_nrf_power_event(NRFX_POWER_USB_EVT_READY);
  }
}
/*---------------------------------------------------------------------------*/
#endif /* NRF_HAS_USB */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */

/*
 * Copyright (c) 2017 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * \addtogroup nrf52840-usb
 * @{
 *
 * \file
 * Implementation of the nRF dongle USB DFU trigger interface.
 * This allows nrfutil to reset the PCA10059 dongle to bootloader mode.
 *
 * Based on lib/nrf52-sdk/components/libraries/bootloader/dfu/nrf_dfu_trigger_usb.c
 *
 */
/*---------------------------------------------------------------------------*/
#include "app_usbd.h"
#include "app_usbd_nrf_dfu_trigger.h"
#include "nrf_gpio.h"
#include "boards.h"
/*---------------------------------------------------------------------------*/
#ifndef BSP_SELF_PINRESET_PIN
#error "This module is intended to be used with boards that have the GP pin shortened with the RESET pin."
#endif
/*---------------------------------------------------------------------------*/
#define DFU_FLASH_PAGE_SIZE  (NRF_FICR->CODEPAGESIZE)
#define DFU_FLASH_PAGE_COUNT (NRF_FICR->CODESIZE)
/*---------------------------------------------------------------------------*/
static uint8_t m_version_string[] = "Contiki-NG DFU";
static app_usbd_nrf_dfu_trigger_nordic_info_t m_dfu_info;
/*---------------------------------------------------------------------------*/
static void
dfu_trigger_evt_handler(app_usbd_class_inst_t const *p_inst,
                        app_usbd_nrf_dfu_trigger_user_event_t event)
{
  UNUSED_PARAMETER(p_inst);

  switch(event) {
  case APP_USBD_NRF_DFU_TRIGGER_USER_EVT_DETACH:
    nrf_gpio_cfg_output(BSP_SELF_PINRESET_PIN);
    nrf_gpio_pin_clear(BSP_SELF_PINRESET_PIN);
    break;
  default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
APP_USBD_NRF_DFU_TRIGGER_GLOBAL_DEF(m_app_dfu,
                                    NRF_DFU_TRIGGER_USB_INTERFACE_NUM,
                                    &m_dfu_info, m_version_string,
                                    dfu_trigger_evt_handler);
/*---------------------------------------------------------------------------*/
void
dfu_trigger_usb_init(void)
{
  app_usbd_class_inst_t const *class_dfu;

  m_dfu_info.wAddress = CODE_START;
  m_dfu_info.wFirmwareSize = CODE_SIZE;
  m_dfu_info.wVersionMajor = 0;
  m_dfu_info.wVersionMinor = 0;
  m_dfu_info.wFirmwareID = 0;
  m_dfu_info.wFlashPageSize = DFU_FLASH_PAGE_SIZE;
  m_dfu_info.wFlashSize = m_dfu_info.wFlashPageSize * DFU_FLASH_PAGE_COUNT;

  class_dfu = app_usbd_nrf_dfu_trigger_class_inst_get(&m_app_dfu);
  app_usbd_class_append(class_dfu);
}
/*---------------------------------------------------------------------------*/

/** @} */

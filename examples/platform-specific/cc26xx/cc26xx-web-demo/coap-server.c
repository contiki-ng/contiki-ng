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
/**
 * \addtogroup cc26xx-web-demo
 * @{
 *
 * \file
 *     A CC26XX-specific CoAP server
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "board-peripherals.h"
#include "rf-core/rf-ble.h"
#include "cc26xx-web-demo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Common resources */
extern coap_resource_t res_leds;

extern coap_resource_t res_batmon_temp;
extern coap_resource_t res_batmon_volt;

extern coap_resource_t res_device_sw;
extern coap_resource_t res_device_hw;
extern coap_resource_t res_device_uptime;
extern coap_resource_t res_device_cfg_reset;

extern coap_resource_t res_parent_rssi;
extern coap_resource_t res_parent_ip;

#if RF_BLE_ENABLED
extern coap_resource_t res_ble_advd;
#endif

extern coap_resource_t res_toggle_red;
extern coap_resource_t res_toggle_green;

/* Board-specific resources */
#if BOARD_SENSORTAG
extern coap_resource_t res_bmp280_temp;
extern coap_resource_t res_bmp280_press;
extern coap_resource_t res_tmp007_amb;
extern coap_resource_t res_tmp007_obj;
extern coap_resource_t res_hdc1000_temp;
extern coap_resource_t res_hdc1000_hum;
extern coap_resource_t res_opt3001_light;
extern coap_resource_t res_mpu_acc_x;
extern coap_resource_t res_mpu_acc_y;
extern coap_resource_t res_mpu_acc_z;
extern coap_resource_t res_mpu_gyro_x;
extern coap_resource_t res_mpu_gyro_y;
extern coap_resource_t res_mpu_gyro_z;
#else
extern coap_resource_t res_toggle_orange;
extern coap_resource_t res_toggle_yellow;
#endif

#if CC26XX_WEB_DEMO_ADC_DEMO
extern coap_resource_t res_adc_dio23;
#endif
/*---------------------------------------------------------------------------*/
const char *coap_server_not_found_msg = "Resource not found";
const char *coap_server_supported_msg = "Supported:"
                                        "text/plain,"
                                        "application/json,"
                                        "application/xml";
/*---------------------------------------------------------------------------*/
static void
start_board_resources(void)
{

  coap_activate_resource(&res_toggle_green, "lt/g");
  coap_activate_resource(&res_toggle_red, "lt/r");
  coap_activate_resource(&res_leds, "lt");

#if BOARD_SENSORTAG
  coap_activate_resource(&res_bmp280_temp, "sen/bar/temp");
  coap_activate_resource(&res_bmp280_press, "sen/bar/pres");
  coap_activate_resource(&res_tmp007_amb, "sen/tmp/amb");
  coap_activate_resource(&res_tmp007_obj, "sen/tmp/obj");
  coap_activate_resource(&res_hdc1000_temp, "sen/hdc/t");
  coap_activate_resource(&res_hdc1000_hum, "sen/hdc/h");
  coap_activate_resource(&res_opt3001_light, "sen/opt/light");
  coap_activate_resource(&res_mpu_acc_x, "sen/mpu/acc/x");
  coap_activate_resource(&res_mpu_acc_y, "sen/mpu/acc/y");
  coap_activate_resource(&res_mpu_acc_z, "sen/mpu/acc/z");
  coap_activate_resource(&res_mpu_gyro_x, "sen/mpu/gyro/x");
  coap_activate_resource(&res_mpu_gyro_y, "sen/mpu/gyro/y");
  coap_activate_resource(&res_mpu_gyro_z, "sen/mpu/gyro/z");
#elif BOARD_SMARTRF06EB
  coap_activate_resource(&res_toggle_yellow, "lt/y");
  coap_activate_resource(&res_toggle_orange, "lt/o");
#endif
}
/*---------------------------------------------------------------------------*/
PROCESS(coap_server_process, "CC26XX CoAP Server");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coap_server_process, ev, data)
{
  PROCESS_BEGIN();

  printf("CC26XX CoAP Server\n");

  coap_activate_resource(&res_batmon_temp, "sen/batmon/temp");
  coap_activate_resource(&res_batmon_volt, "sen/batmon/voltage");

#if CC26XX_WEB_DEMO_ADC_DEMO
  coap_activate_resource(&res_adc_dio23, "sen/adc/dio23");
#endif

  coap_activate_resource(&res_device_hw, "dev/mdl/hw");
  coap_activate_resource(&res_device_sw, "dev/mdl/sw");
  coap_activate_resource(&res_device_uptime, "dev/uptime");
  coap_activate_resource(&res_device_cfg_reset, "dev/cfg_reset");

  coap_activate_resource(&res_parent_rssi, "net/parent/RSSI");
  coap_activate_resource(&res_parent_ip, "net/parent/IPv6");

#if RF_BLE_ENABLED
  coap_activate_resource(&res_ble_advd, "dev/ble_advd");
#endif

  start_board_resources();

  /* Define application-specific events here. */
  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */

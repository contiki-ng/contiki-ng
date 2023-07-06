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
#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_
/*---------------------------------------------------------------------------*/
/* Change to match your configuration */
#define IEEE802154_CONF_PANID            0xABCD
#define IEEE802154_CONF_DEFAULT_CHANNEL      26
#define RF_BLE_CONF_ENABLED                   1
/*---------------------------------------------------------------------------*/

/* Enable TCP */
#define UIP_CONF_TCP 1

/* Enable/Disable Components of this Demo */
#define CC26XX_WEB_DEMO_CONF_MQTT_CLIENT      1
#define CC26XX_WEB_DEMO_CONF_6LBR_CLIENT      ROUTING_CONF_RPL_CLASSIC
#define CC26XX_WEB_DEMO_CONF_COAP_SERVER      1
#define CC26XX_WEB_DEMO_CONF_NET_UART         1

/*
 * ADC sensor functionality. To test this, an external voltage source should be
 * connected to DIO23
 * Enable/Disable DIO23 ADC reading by setting CC26XX_WEB_DEMO_CONF_ADC_DEMO
 */
#define CC26XX_WEB_DEMO_CONF_ADC_DEMO         0
/*---------------------------------------------------------------------------*/
/*
 * Change to 1 if you are using an older CC2650 Sensortag (look for Rev: 1.2
 * printed on the PCB, or for a sticker reading "HW Rev 1.2.0").
 *
 * This may be the case if you are getting this error:
 * "Could not open flash to load config"
 * when your sensortag is starting up.
 */
#define SENSORTAG_CC2650_REV_1_2_0            0
/*---------------------------------------------------------------------------*/
/* Enable the ROM bootloader */
#define CCFG_CONF_ROM_BOOTLOADER_ENABLE       1
/*---------------------------------------------------------------------------*/
/*
 * Shrink the size of the uIP buffer, routing table and ND cache.
 * Set the TCP MSS
 */
#define UIP_CONF_BUFFER_SIZE                500
#define NETSTACK_MAX_ROUTE_ENTRIES            5
#define NBR_TABLE_CONF_MAX_NEIGHBORS          5
#define UIP_CONF_TCP_MSS                    128
/*---------------------------------------------------------------------------*/
#endif /* PROJECT_CONF_H_ */
/*---------------------------------------------------------------------------*/

/*
 * Copyright (c) 2015, Texas Instruments Incorporated - http://www.ti.com/
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
/* Platform configuration */
#define BOARD_CONF_SENSORS_DISABLE                  1
#define WATCHDOG_CONF_DISABLE                       1
/*---------------------------------------------------------------------------*/
/* Nestack configuration */
#define IEEE802154_CONF_PANID                       0xABCD
#define IEEE802154_CONF_DEFAULT_CHANNEL             25
//#define RF_CONF_MODE                                RF_MODE_SUB_1_GHZ
#define RF_CONF_MODE                                RF_MODE_2_4_GHZ
#define RF_CONF_BLE_BEACON_ENABLE                   0
/*---------------------------------------------------------------------------*/
/* TI drivers configuration */
#define TI_SPI_CONF_ENABLE                          0
#define TI_I2C_CONF_ENABLE                          0
/*---------------------------------------------------------------------------*/
/* For very sleepy operation */
#define UIP_DS6_CONF_PERIOD                         CLOCK_SECOND
#define UIP_CONF_TCP                                0
#define RPL_CONF_LEAF_ONLY                          1

/*
 * We'll fail without RPL probing, so turn it on explicitly even though it's
 * on by default
 */
#define RPL_CONF_WITH_PROBING                       1
/*---------------------------------------------------------------------------*/
#endif /* PROJECT_CONF_H_ */
/*---------------------------------------------------------------------------*/

/*
 * Copyright (c) 2014, Thingsquare, http://www.thingsquare.com/.
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
 *
 * This file is part of the Contiki operating system.
 *
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup openmote-b-antenna
 * @{
 *
 * Driver for the OpenMote-B RF switch. The 2.4 GHz SMA connector can be
 * connected to either:
 * - CC2538 radio, configured through antenna_select_cc2538()
 * - AT86RF215 radio, configured through antenna_select_at86rf215()
 *
 * Note that the sub-GHz SMA connector on the board is always connected to
 * the AT86RF215 radio.
 * @{
 *
 * \file
 * Driver implementation for the OpenMote-B antenna switch
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/gpio.h"
#include "antenna.h"
/*---------------------------------------------------------------------------*/
void
antenna_init(void)
{
  /* Configure the GPIO pins as output */
  GPIO_SOFTWARE_CONTROL(ANTENNA_BSP_RADIO_BASE, ANTENNA_BSP_RADIO_24GHZ_CC2538);
  GPIO_SOFTWARE_CONTROL(ANTENNA_BSP_RADIO_BASE, ANTENNA_BSP_RADIO_24GHZ_AT86RF215);
  GPIO_SET_OUTPUT(ANTENNA_BSP_RADIO_BASE, ANTENNA_BSP_RADIO_24GHZ_CC2538);
  GPIO_SET_OUTPUT(ANTENNA_BSP_RADIO_BASE, ANTENNA_BSP_RADIO_24GHZ_AT86RF215);
}
/*---------------------------------------------------------------------------*/
void
antenna_select_cc2538(void)
{
  GPIO_SET_PIN(ANTENNA_BSP_RADIO_BASE, ANTENNA_BSP_RADIO_24GHZ_CC2538);
  GPIO_CLR_PIN(ANTENNA_BSP_RADIO_BASE, ANTENNA_BSP_RADIO_24GHZ_AT86RF215);
}
/*---------------------------------------------------------------------------*/
void
antenna_select_at86rf215(void)
{
  GPIO_SET_PIN(ANTENNA_BSP_RADIO_BASE, ANTENNA_BSP_RADIO_24GHZ_AT86RF215);
  GPIO_CLR_PIN(ANTENNA_BSP_RADIO_BASE, ANTENNA_BSP_RADIO_24GHZ_CC2538);
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

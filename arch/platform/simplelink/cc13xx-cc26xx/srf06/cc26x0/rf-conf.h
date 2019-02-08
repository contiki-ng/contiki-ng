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
 * \addtogroup launchpad-peripherals
 * @{
 *
 * \file
 *        Header file with board-specific RF configurations.
 * \author
 *        Texas Instruments <e2e.ti.com>
 * \note
 *        This file should not be included directly
 */
/*---------------------------------------------------------------------------*/
#ifndef RF_CONF_H_
#define RF_CONF_H_
/*---------------------------------------------------------------------------*/
#include "rf/rf.h"
/*---------------------------------------------------------------------------*/
/**
 * \name  Board-specific front-end mode configurations for the 2.4 GHz path
 *        on the radio.
 *
 * These are the following front-end mode configurations for the
 * CC2650DK-7ID board:
 *  - 2.4 GHz: differential and internal bias
 *
 * @{
 */
#define RF_2_4_GHZ_CONF_FRONT_END_MODE    RF_FRONT_END_MODE_DIFFERENTIAL
#define RF_2_4_GHZ_CONF_BIAS_MODE         RF_BIAS_MODE_INTERNAL
/** @} */
/*---------------------------------------------------------------------------*/
#endif /* RF_CONF_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 */

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
 * \addtogroup cc13xx-cc26xx-cpu
 * @{
 *
 * \defgroup cc13xx-cc26xx-rf RF specific files for CC13xx/CC26xx
 *
 * @{
 *
 * \file
 *        Header file of common CC13xx/CC26xx RF functionality.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef RF_CORE_H_
#define RF_CORE_H_
/*---------------------------------------------------------------------------*/
#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
/*
 * \name  Abstract values to specify either the minimum or the maximum
 *        available TX power setting in dBm.
 *
 * @{
 */
#define RF_TXPOWER_MIN_DBM     RF_TxPowerTable_MIN_DBM
#define RF_TXPOWER_MAX_DBM     RF_TxPowerTable_MAX_DBM
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name  Different modes the RF can operate on, denoted by which frequency
 *        band said mode operates on. Currently supports the following modes:
 *        - Sub-1 GHz, called prop-mode
 *        - 2.4 GHz,   called ieee-mode
 *
 * @{
 */
#define RF_MODE_SUB_1_GHZ      (1 << 0)
#define RF_MODE_2_4_GHZ        (1 << 1)

/* Bitmask of supported RF modes */
#define RF_MODE_BM             (RF_MODE_SUB_1_GHZ | \
                                RF_MODE_2_4_GHZ)
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name  The different front-end modes the CC13xx/CC26xx devices support. The
 *        front-end mode can be configured independently of the bias mode. The
 *        two types of modes are as follows:
 *        - Differential: Both RF_P and RF_N are used as a differential RF
 *          interface.
 *        - Single ended: Either the RF_P pin or the RF_N pin is used as the
 *          RF path.
 *
 * @{
 */
/* Available front-end mode configurations */
#define RF_FRONT_END_MODE_DIFFERENTIAL      0
#define RF_FRONT_END_MODE_SINGLE_ENDED_RFP  1
#define RF_FRONT_END_MODE_SINGLE_ENDED_RFN  2
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name  The different bias modes the CC13xx/CC26xx devices support. The
 *        bias mode can be configured independently of the front-end mode. The
 *        two different modes are as follows:
 *        - Internal bias: the LNA is biased by an internal bias.
 *        - External bias: the LNA is biased by an external bias.
 *
 * @{
 */
/* Available bias mode configurations */
#define RF_BIAS_MODE_INTERNAL       0
#define RF_BIAS_MODE_EXTERNAL       1
/** @} */
/*---------------------------------------------------------------------------*/
#endif /* RF_CORE_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 */

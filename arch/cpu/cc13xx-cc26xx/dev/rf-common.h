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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup simplelink
 * @{
 *
 * \defgroup rf-common Common functionality fpr the CC13xx/CC26xx RF
 *
 * @{
 *
 * \file
 * Header file of common CC13xx/CC26xx RF functionality
 */
/*---------------------------------------------------------------------------*/
#ifndef RF_COMMON_H_
#define RF_COMMON_H_
/*---------------------------------------------------------------------------*/
/* Contiki API */
#include <sys/rtimer.h>
#include <dev/radio.h>
/*---------------------------------------------------------------------------*/
/* Standard library */
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#ifdef RF_CORE_CONF_CHANNEL
#   define RF_CORE_CHANNEL  RF_CORE_CONF_CHANNEL
#else
#   define RF_CORE_CHANNEL  25
#endif
/*---------------------------------------------------------------------------*/
typedef enum {
    CMD_RESULT_ERROR = 0,
    CMD_RESULT_OK = 1,
} CmdResult;
/*---------------------------------------------------------------------------*/
typedef struct {
  radio_value_t dbm;
  uint16_t      power; ///< Value for the .txPower field
} RF_TxPower;

#define TX_POWER_UNKNOWN  0xFFFF
/*---------------------------------------------------------------------------*/
#define RSSI_UNKNOWN      -128
/*---------------------------------------------------------------------------*/
PROCESS_NAME(RF_coreProcess);
/*---------------------------------------------------------------------------*/
#endif /* RF_COMMON_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

/*
 * Copyright (c) 2020, Institute of Electronics and Computer Science (EDI)
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
 * \addtogroup cc13xx-cc26xx-rf
 * @{
 *
 * \defgroup cc13xx-cc26xx-rf-radio-mode Generic radio mode API
 *
 * @{
 *
 * \file
 *        Header file of the generic radio mode API.
 * \author
 *        Atis Elsts <atis.elsts@edi.lv>
 */
/*---------------------------------------------------------------------------*/
#ifndef RF_RADIO_MODE_H_
#define RF_RADIO_MODE_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/ctimer.h"
/*---------------------------------------------------------------------------*/
#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
/**
 * \name Radio mode API
 *
 * @{
 */
typedef struct {
  /* RF driver */
  RF_Handle rf_handle;

  /* Are we currently in poll mode? */
  bool poll_mode;

  /* RAT Overflow Upkeep */
  struct {
    struct ctimer overflow_timer;
    rtimer_clock_t last_overflow;
    volatile uint32_t overflow_count;
  } rat;

  bool (* rx_is_active)(void);

} simplelink_radio_mode_t;

extern simplelink_radio_mode_t *radio_mode;
/** @} */
/*---------------------------------------------------------------------------*/
#endif /* RF_RADIO_MODE_H__ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

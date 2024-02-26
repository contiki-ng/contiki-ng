/*
 * Copyright (C) 2024 Marcel Graber <marcel@clever.design>
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 * \addtogroup nrf-sys System drivers
 * @{
 *
 * \file
 *  Configuration for the nrf platform
 */
#ifndef LPM_H
#define LPM_H

#include "contiki.h"

#include "energest.h"
#include "critical.h"
#include "process.h"

/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define LPM_MODE_AWAKE         0
#define LPM_MODE_SLEEP         1
#define LPM_MODE_DEEP_SLEEP    2

#ifndef LPM_MODE_MAX_SUPPORTED_CONF
#define LPM_MODE_MAX_SUPPORTED LPM_MODE_DEEP_SLEEP
#else
#define LPM_MODE_MAX_SUPPORTED LPM_MODE_MAX_SUPPORTED_CONF
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Drop the cortex to sleep / deep sleep and shut down peripherals
 *
 * Whether the cortex will drop to sleep or deep sleep is configurable. The
 * exact peripherals which will be shut down is also configurable
 */
void lpm_drop(void);

/**
 * \brief Enter sleep mode
 */
void lpm_sleep(void);

/**
 * \brief Put the chip in shutdown power mode
 * \param wakeup_pin The GPIO pin which will wake us up. Must be IOID_0 etc...
 * \param io_pull Pull configuration for the shutdown pin: IOC_NO_IOPULL,
 *        IOC_IOPULL_UP or IOC_IOPULL_DOWN
 * \param wake_on High or Low (IOC_WAKE_ON_LOW or IOC_WAKE_ON_HIGH)
 */
void lpm_shutdown(uint32_t wakeup_pin, uint32_t io_pull, uint32_t wake_on);

/**
 * \brief Sets an IOID to a default state
 * \param ioid IOID_0...
 *
 * This will set ioid to sw control, input, no pull. Input buffer and output
 * driver will both be disabled
 *
 * The function will do nothing if ioid == IOID_UNUSED, so the caller does not
 * have to check board configuration before calling this.
 */
void lpm_pin_set_default_state(uint32_t ioid);
/*---------------------------------------------------------------------------*/
/* /\*---------------------------------------------------------------------------*\/ */
/* /\** */
/*  * @brief Stop and wait for an interrupt */
/*  *\/ */
/* static inline void */
/* lpm_drop(void) */
/* { */
/*   int_master_status_t status; */
/*   int abort; */
/*   status = critical_enter(); */
/*   abort = process_nevents(); */
/*   if(!abort) { */
/*     ENERGEST_SWITCH(ENERGEST_TYPE_CPU, ENERGEST_TYPE_LPM); */
/*     __WFI(); */
/*     ENERGEST_SWITCH(ENERGEST_TYPE_LPM, ENERGEST_TYPE_CPU); */
/*   } */
/*   critical_exit(status); */
/* } */
/* /\*---------------------------------------------------------------------------*\/ */
#endif /* LPM_H */

/** @} */

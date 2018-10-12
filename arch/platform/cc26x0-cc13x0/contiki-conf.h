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
 * \addtogroup cc26xx-srf-tag
 * @{
 *
 * \file
 *  Configuration for the cc26x0-cc13x0 platform
 */
#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

#include <stdint.h>
#include <inttypes.h>
/*---------------------------------------------------------------------------*/
/* Include Project Specific conf */
#ifdef PROJECT_CONF_PATH
#include PROJECT_CONF_PATH
#endif /* PROJECT_CONF_PATH */
/*---------------------------------------------------------------------------*/
#include "cc13xx-cc26xx-def.h"
/*---------------------------------------------------------------------------*/
/**
 * \name Button configurations
 *
 * @{
 */

/* Notify various examples that we have Buttons */
#define PLATFORM_HAS_BUTTON      1
#define PLATFORM_SUPPORTS_BUTTON_HAL 1

/** @} */
/*---------------------------------------------------------------------------*/
/* Platform-specific define to signify sensor reading failure */
#define CC26XX_SENSOR_READING_ERROR        0x80000000
/*---------------------------------------------------------------------------*/
/* Include CPU-related configuration */
#include "cc13xx-cc26xx-conf.h"
/*---------------------------------------------------------------------------*/
/* board.h assumes that basic configuration is done */
#include "board.h"
/*---------------------------------------------------------------------------*/
#endif /* CONTIKI_CONF_H */

/** @} */

/*
 * Copyright (c) 2017, RISE SICS AB
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
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
 * \addtogroup efr32tb
 * @{
 *
 * \file
 *  Configuration for the efr32tb platform
 */
#ifndef CONTIKI_CONF_H_
#define CONTIKI_CONF_H_

#include <stdint.h>
#include <inttypes.h>
/*---------------------------------------------------------------------------*/
/* Include project specific configuration */
#ifdef PROJECT_CONF_PATH
#include PROJECT_CONF_PATH
#endif /* PROJECT_CONF_PATH */
/*---------------------------------------------------------------------------*/
#include "efr32-def.h"
/*---------------------------------------------------------------------------*/
/* Disable the stack check library for now */
#define PLATFORM_CONF_SUPPORTS_STACK_CHECK  0
/*---------------------------------------------------------------------------*/
/**
 * \name Button configurations
 *
 * @{
 */

/* Notify various examples that we have Buttons */
#define PLATFORM_SUPPORTS_BUTTON_HAL 1

/** @} */
/*---------------------------------------------------------------------------*/
/* Board specific configuration */
#include "board.h"
/*---------------------------------------------------------------------------*/
/* Include CPU-related configuration */
#include "efr32-conf.h"
/*---------------------------------------------------------------------------*/
#endif /* CONTIKI_CONF_H_ */
/** @} */

/*
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
/** \addtogroup sys
 * @{
 *
 * \defgroup interrupts Master interrupt manipulation
 * @{
 *
 * These functions can be used to manipulate the master interrupt in a
 * platform-independent fashion
 */
/*---------------------------------------------------------------------------*/
#ifndef INT_MASTER_H_
#define INT_MASTER_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <stdbool.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#ifdef INT_MASTER_CONF_STATUS_DATATYPE
#define INT_MASTER_STATUS_DATATYPE INT_MASTER_CONF_STATUS_DATATYPE
#else
#define INT_MASTER_STATUS_DATATYPE uint32_t
#endif

/**
 * \brief Master interrupt state representation data type
 *
 * It is possible for the platform code to change this datatype by defining
 * INT_MASTER_CONF_STATUS_DATATYPE
 */
typedef INT_MASTER_STATUS_DATATYPE int_master_status_t;
/*---------------------------------------------------------------------------*/
/**
 * \brief Enable the master interrupt
 *
 * The platform developer must provide this function
 */
void int_master_enable(void);

/**
 * \brief Disable the master interrupt
 * \return The status of the master interrupt before disabling it
 *
 * This function will return the status of the master interrupt as it was
 * before it got disabled.
 *
 * The semantics of the return value are entirely platform-specific. The
 * calling code should not try to determine whether the master interrupt was
 * previously enabled/disabled by interpreting the return value of this
 * function. The return value should only be used as an argument to
 * int_master_status_set()
 *
 * To determine the status of the master interrupt in a platform-independent
 * fashion you should use int_master_is_enabled().
 *
 * The platform developer must provide this function
 */
int_master_status_t int_master_read_and_disable(void);

/**
 * \brief Set the status of the master interrupt
 * \param status The new status
 *
 * The semantics of \e status are platform-dependent. Normally, the argument
 * provided to this function will be a value previously retrieved through a
 * call to int_master_read_and_disable()
 *
 * The platform developer must provide this function
 */
void int_master_status_set(int_master_status_t status);

/**
 * \brief Retrieve the status of the master interrupt
 * \retval false Interrupts are disabled
 * \retval true Interrupts are enabled
 *
 * This function can be used to retrieve the status of the master interrupt
 * in a platform-independent fashion.
 *
 * The platform developer must provide this function
 */
bool int_master_is_enabled(void);
/*---------------------------------------------------------------------------*/
#endif /* INT_MASTER_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

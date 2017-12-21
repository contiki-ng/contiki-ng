/*
 * Copyright (c) 2015, Yanzi Networks AB.
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
 * \addtogroup lwm2m
 * @{
 */

/**
 * \file
 *         Header file for the Contiki OMA LWM2M device
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef LWM2M_DEVICE_H_
#define LWM2M_DEVICE_H_

#include "contiki.h"

#define LWM2M_DEVICE_MANUFACTURER_ID            0
#define LWM2M_DEVICE_MODEL_NUMBER_ID            1
#define LWM2M_DEVICE_SERIAL_NUMBER_ID           2
#define LWM2M_DEVICE_FIRMWARE_VERSION_ID        3
#define LWM2M_DEVICE_REBOOT_ID                  4
#define LWM2M_DEVICE_FACTORY_DEFAULT_ID         5
#define LWM2M_DEVICE_AVAILABLE_POWER_SOURCES    6
/* These do have multiple instances */
#define LWM2M_DEVICE_POWER_SOURCE_VOLTAGE       7
#define LWM2M_DEVICE_POWER_SOURCE_CURRENT       8
#define LWM2M_DEVICE_BATTERY_LEVEL              9

#define LWM2M_DEVICE_ERROR_CODE                11
#define LWM2M_DEVICE_TIME_ID                   13
#define LWM2M_DEVICE_TYPE_ID                   17

#ifndef LWM2M_DEVICE_MODEL_NUMBER
#ifdef BOARD_STRING
#define LWM2M_DEVICE_MODEL_NUMBER BOARD_STRING
#endif /* BOARD_STRING */
#endif /* LWM2M_DEVICE_MODEL_NUMBER */

#ifndef LWM2M_DEVICE_FIRMWARE_VERSION
#ifdef CONTIKI_VERSION_STRING
#define LWM2M_DEVICE_FIRMWARE_VERSION CONTIKI_VERSION_STRING
#endif /* CONTIKI_VERSION_STRING */
#endif /* LWM2M_DEVICE_FIRMWARE_VERSION */

int32_t lwm2m_device_get_time(void);
void    lwm2m_device_set_time(int32_t time);

void lwm2m_device_init(void);

#endif /* LWM2M_DEVICE_H_ */
/** @} */

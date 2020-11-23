/*
 * Copyright (c) 2020, George Oikonomou - http://www.spd.gr
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
#ifndef TARGET_CONF_H_
#define TARGET_CONF_H_
/*---------------------------------------------------------------------------*/
/* Platform-specific includes */
#include "rf-core/rf-ble.h"

#include "ti-lib.h"
/*---------------------------------------------------------------------------*/
/* Platform-specific example configuration */
#define CC26XX_DEMO_TRIGGER_1     BOARD_BUTTON_HAL_INDEX_KEY_LEFT
#define CC26XX_DEMO_TRIGGER_2     BOARD_BUTTON_HAL_INDEX_KEY_RIGHT

#if BOARD_SENSORTAG
#define CC26XX_DEMO_TRIGGER_3     BOARD_BUTTON_HAL_INDEX_REED_RELAY
#endif
/*---------------------------------------------------------------------------*/
/* Platform-specific sensor reading error macros */
#define HDC_1000_READING_ERROR    CC26XX_SENSOR_READING_ERROR
#define TMP_007_READING_ERROR     CC26XX_SENSOR_READING_ERROR
#define BMP_280_READING_ERROR     CC26XX_SENSOR_READING_ERROR
#define OPT_3001_READING_ERROR    CC26XX_SENSOR_READING_ERROR
/*---------------------------------------------------------------------------*/
#endif /* TARGET_CONF_H_ */
/*---------------------------------------------------------------------------*/

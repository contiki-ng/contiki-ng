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
/* Platform-specific example configuration */
#define CC26XX_DEMO_TRIGGER_1       BUTTON_HAL_ID_KEY_LEFT
#define CC26XX_DEMO_TRIGGER_2       BUTTON_HAL_ID_KEY_RIGHT

#if BOARD_SENSORTAG
#define CC26XX_DEMO_TRIGGER_3       BUTTON_HAL_ID_REED_RELAY
#endif
/*---------------------------------------------------------------------------*/
#define TMP_007_SENSOR_TYPE_AMBIENT TMP_007_TYPE_AMBIENT
#define TMP_007_SENSOR_TYPE_OBJECT  TMP_007_TYPE_OBJECT
#define TMP_007_SENSOR_TYPE_ALL     TMP_007_TYPE_ALL
/*---------------------------------------------------------------------------*/
#endif /* TARGET_CONF_H_ */
/*---------------------------------------------------------------------------*/

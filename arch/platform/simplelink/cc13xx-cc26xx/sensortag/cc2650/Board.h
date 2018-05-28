/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __BOARD_H
#define __BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ti/drivers/Power.h>

#include "CC2650STK.h"

/* These #defines allow us to reuse TI-RTOS across other device families */
#define     Board_LED1              Board_STK_LED1
#define     Board_LED2              Board_STK_LED2
#define     Board_LED0              Board_LED2

#define     Board_BUTTON0           Board_KEY_LEFT
#define     Board_BUTTON1           Board_KEY_RIGHT

#define     Board_I2C0              Board_I2C
#define     Board_I2C_TMP           Board_I2C0
#define     Board_UART0             Board_UART
#define     Board_AES0              Board_AES
#define     Board_WATCHDOG0         CC2650STK_WATCHDOG0

#define     Board_initGeneral() { \
    Power_init(); \
    if (PIN_init(BoardGpioInitTable) != PIN_SUCCESS) \
        {System_abort("Error with PIN_init\n"); \
    } \
}

#define     Board_initGPIO()
#define     Board_initPWM()        PWM_init()
#define     Board_initI2C()         I2C_init()
#define     Board_initSPI()         SPI_init()
#define     Board_initUART()        UART_init()
#define     Board_initWatchdog()    Watchdog_init()
#define     GPIO_toggle(n)
#define     GPIO_write(n,m)

/* Board specific I2C addresses */

/* Interface #0 */
#define     Board_HDC1000_ADDR      (0x43)
#define     Board_TMP007_ADDR       (0x44)
#define     Board_OPT3001_ADDR      (0x45)
#define     Board_BMP280_ADDR       (0x77)

/* Interface #1 */
#define     Board_MPU9250_ADDR      (0x68)
#define     Board_MPU9250_MAG_ADDR  (0x0C)

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */

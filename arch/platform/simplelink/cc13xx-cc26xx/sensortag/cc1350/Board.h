/*
 * Copyright (c) 2015-2019, Texas Instruments Incorporated
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

#include <ti/drivers/Board.h>
#include "CC1350STK.h"

#define Board_CC1350STK
#define BOARD_STRING            "TI CC1350 SensorTag"

#define Board_initGeneral()      Board_init()  /* deprecated */
#define Board_shutDownExtFlash() CC1350STK_shutDownExtFlash()
#define Board_wakeUpExtFlash()   CC1350STK_wakeUpExtFlash()

/* These #defines allow us to reuse TI-RTOS across other device families */
#define Board_ADC0              CC1350STK_ADC0
#define Board_ADC1              CC1350STK_ADC1

#define Board_ADCBUF0           CC1350STK_ADCBUF0
#define Board_ADCBUF0CHANNEL0   CC1350STK_ADCBUF0CHANNEL0
#define Board_ADCBUF0CHANNEL1   CC1350STK_ADCBUF0CHANNEL1

#define Board_CRYPTO0           CC1350STK_CRYPTO0
#define Board_AESCCM0           CC1350STK_AESCCM0
#define Board_AESGCM0           CC1350STK_AESGCM0
#define Board_AESCBC0           CC1350STK_AESCBC0
#define Board_AESCTR0           CC1350STK_AESCTR0
#define Board_AESECB0           CC1350STK_AESECB0
#define Board_AESCTRDRBG0       CC1350STK_AESCTRDRBG0
#define Board_TRNG0             CC1350STK_TRNG0

#define Board_DIO16_TDO         CC1350STK_DIO16_TDO
#define Board_DIO17_TDI         CC1350STK_DIO17_TDI
#define Board_DIO22             CC1350STK_DIO22

#define Board_DIO23_ANALOG      CC1350STK_DIO23_ANALOG
#define Board_DIO24_ANALOG      CC1350STK_DIO24_ANALOG
#define Board_DIO25_ANALOG      CC1350STK_DIO25_ANALOG
#define Board_DIO26_ANALOG      CC1350STK_DIO26_ANALOG
#define Board_DIO27_ANALOG      CC1350STK_DIO27_ANALOG
#define Board_DIO28_ANALOG      CC1350STK_DIO28_ANALOG
#define Board_DIO29_ANALOG      CC1350STK_DIO29_ANALOG
#define Board_DIO30_ANALOG      CC1350STK_DIO30_ANALOG

#define Board_GPIO_BUTTON0      CC1350STK_GPIO_S1
#define Board_GPIO_BUTTON1      CC1350STK_GPIO_S2
#define Board_GPIO_BTN1         CC1350STK_GPIO_S1
#define Board_GPIO_BTN2         CC1350STK_GPIO_S2
#define Board_GPIO_LED0         CC1350STK_GPIO_LED_RED
#define Board_GPIO_LED1         CC1350STK_GPIO_LED_RED
#define Board_GPIO_RLED         CC1350STK_GPIO_LED_RED
#define Board_GPIO_LED_ON       CC1350STK_GPIO_LED_ON
#define Board_GPIO_LED_OFF      CC1350STK_GPIO_LED_OFF

#define Board_GPTIMER0A         CC1350STK_GPTIMER0A
#define Board_GPTIMER0B         CC1350STK_GPTIMER0B
#define Board_GPTIMER1A         CC1350STK_GPTIMER1A
#define Board_GPTIMER1B         CC1350STK_GPTIMER1B
#define Board_GPTIMER2A         CC1350STK_GPTIMER2A
#define Board_GPTIMER2B         CC1350STK_GPTIMER2B
#define Board_GPTIMER3A         CC1350STK_GPTIMER3A
#define Board_GPTIMER3B         CC1350STK_GPTIMER3B

#define Board_NVSINTERNAL       CC1350STK_NVSCC26XX0
#define Board_NVSEXTERNAL       CC1350STK_NVSSPI25X0

#define Board_I2C0              CC1350STK_I2C0
#define Board_I2C1              CC1350STK_I2C1

#define Board_PIN_BUTTON0       CC1350STK_PIN_BTN1
#define Board_PIN_BUTTON1       CC1350STK_PIN_BTN2
#define Board_PIN_BTN1          CC1350STK_PIN_BTN1
#define Board_PIN_BTN2          CC1350STK_PIN_BTN2
#define Board_PIN_LED0          CC1350STK_PIN_RLED
#define Board_PIN_LED1          CC1350STK_PIN_RLED
#define Board_PIN_LED2          CC1350STK_PIN_RLED
#define Board_PIN_RLED          CC1350STK_PIN_RLED

#define Board_KEY_LEFT          CC1350STK_KEY_LEFT
#define Board_KEY_RIGHT         CC1350STK_KEY_RIGHT
#define Board_RELAY             CC1350STK_RELAY

#define Board_BUZZER            CC1350STK_BUZZER
#define Board_BUZZER_ON         CC1350STK_LED_ON
#define Board_BUZZER_OFF        CC1350STK_LED_OFF

#define Board_MIC_POWER         CC1350STK_MIC_POWER
#define Board_MIC_POWER_OM      CC1350STK_MIC_POWER_ON
#define Board_MIC_POWER_OFF     CC1350STK_MIC_POWER_OFF

#define Board_MPU_INT           CC1350STK_MPU_INT
#define Board_MPU_POWER         CC1350STK_MPU_POWER
#define Board_MPU_POWER_OFF     CC1350STK_MPU_POWER_OFF
#define Board_MPU_POWER_ON      CC1350STK_MPU_POWER_ON

#define Board_TMP_RDY           CC1350STK_TMP_RDY

#define Board_I2S0              CC1350STK_I2S0
#define Board_I2S_ADO           CC1350STK_I2S_ADO
#define Board_I2S_ADI           CC1350STK_I2S_ADI
#define Board_I2S_BCLK          CC1350STK_I2S_BCLK
#define Board_I2S_MCLK          CC1350STK_I2S_MCLK
#define Board_I2S_WCLK          CC1350STK_I2S_WCLK

#define Board_PWM0              CC1350STK_PWM0
#define Board_PWM1              CC1350STK_PWM0
#define Board_PWM2              CC1350STK_PWM2
#define Board_PWM3              CC1350STK_PWM3
#define Board_PWM4              CC1350STK_PWM4
#define Board_PWM5              CC1350STK_PWM5
#define Board_PWM6              CC1350STK_PWM6
#define Board_PWM7              CC1350STK_PWM7

#define Board_SPI0              CC1350STK_SPI0
#define Board_SPI0_MISO         CC1350STK_SPI0_MISO
#define Board_SPI0_MOSI         CC1350STK_SPI0_MOSI
#define Board_SPI0_CLK          CC1350STK_SPI0_CLK
#define Board_SPI0_CSN          CC1350STK_SPI0_CSN
#define Board_SPI1              CC1350STK_SPI1
#define Board_SPI1_MISO         CC1350STK_SPI1_MISO
#define Board_SPI1_MOSI         CC1350STK_SPI1_MOSI
#define Board_SPI1_CLK          CC1350STK_SPI1_CLK
#define Board_SPI1_CSN          CC1350STK_SPI1_CSN
#define Board_SPI_FLASH_CS      CC1350STK_SPI_FLASH_CS
#define Board_FLASH_CS_ON       (0)
#define Board_FLASH_CS_OFF      (1)

#define Board_UART0             CC1350STK_UART0

#define Board_WATCHDOG0         CC1350STK_WATCHDOG0

/* Board specific I2C addresses */
#define Board_BMP280_ADDR       (0x77)
#define Board_HDC1000_ADDR      (0x43)
#define Board_MPU9250_ADDR      (0x68)
#define Board_MPU9250_MAG_ADDR  (0x0C)
#define Board_OPT3001_ADDR      (0x45)
#define Board_TMP_ADDR          (0x44)

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */

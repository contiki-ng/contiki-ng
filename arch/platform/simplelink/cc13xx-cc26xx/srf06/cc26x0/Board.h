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
#include "CC2650DK_7ID.h"

#define Board_CC2650DK_7ID
#define BOARD_STRING            "TI SmartRF06EB + CC26x0 EM"

#define Board_initGeneral()      Board_init()  /* deprecated */
#define Board_shutDownExtFlash() CC2650DK_7ID_shutDownExtFlash()
#define Board_wakeUpExtFlash()   CC2650DK_7ID_wakeUpExtFlash()

/* These #defines allow us to reuse TI-RTOS across other device families */

#define Board_ADCALS            CC2650DK_7ID_ADCALS

#define Board_ADC0              CC2650DK_7ID_ADCVDDS
#define Board_ADC1              CC2650DK_7ID_ADCALS

#define Board_ADCBUF0           CC2650DK_7ID_ADCBUF0
#define Board_ADCBUF0CHANNEL0   CC2650DK_7ID_ADCBUF0CHANNELVDDS
#define Board_ADCBUF0CHANNEL1   CC2650DK_7ID_ADCBUF0CHANNELADCALS

#define Board_CRYPTO0           CC2650DK_7ID_CRYPTO0
#define Board_AESCCM0           CC2650DK_7ID_AESCCM0
#define Board_AESGCM0           CC2650DK_7ID_AESGCM0
#define Board_AESCBC0           CC2650DK_7ID_AESCBC0
#define Board_AESCTR0           CC2650DK_7ID_AESCTR0
#define Board_AESECB0           CC2650DK_7ID_AESECB0
#define Board_AESCTRDRBG0       CC2650DK_7ID_AESCTRDRBG0
#define Board_TRNG0             CC2650DK_7ID_TRNG0

#define Board_DIO0              CC2650DK_7ID_DIO0
#define Board_DIO1_RFSW         CC2650DK_7ID_DIO1_RFSW
#define Board_DIO12             CC2650DK_7ID_DIO12
#define Board_DIO15             CC2650DK_7ID_DIO15
#define Board_DIO16_TDO         CC2650DK_7ID_DIO16_TDO
#define Board_DIO17_TDI         CC2650DK_7ID_DIO17_TDI
#define Board_DIO21             CC2650DK_7ID_DIO21
#define Board_DIO22             CC2650DK_7ID_DIO22

#define Board_DIO23_ANALOG      CC2650DK_7ID_DIO23_ANALOG
#define Board_DIO24_ANALOG      CC2650DK_7ID_DIO24_ANALOG
#define Board_DIO25_ANALOG      CC2650DK_7ID_DIO25_ANALOG
#define Board_DIO26_ANALOG      CC2650DK_7ID_DIO26_ANALOG
#define Board_DIO27_ANALOG      CC2650DK_7ID_DIO27_ANALOG
#define Board_DIO28_ANALOG      CC2650DK_7ID_DIO28_ANALOG
#define Board_DIO29_ANALOG      CC2650DK_7ID_DIO29_ANALOG
#define Board_DIO30_ANALOG      CC2650DK_7ID_DIO30_ANALOG

#define Board_GPIO_BTN0         CC2650DK_7ID_PIN_KEY_SELECT
#define Board_GPIO_BTN1         CC2650DK_7ID_PIN_KEY_UP
#define Board_GPIO_BTN2         CC2650DK_7ID_PIN_KEY_DOWN
#define Board_GPIO_BTN3         CC2650DK_7ID_PIN_KEY_LEFT
#define Board_GPIO_BTN4         CC2650DK_7ID_PIN_KEY_RIGHT
#define Board_GPIO_LED0         CC2650DK_7ID_PIN_LED1
#define Board_GPIO_LED1         CC2650DK_7ID_PIN_LED2
#define Board_GPIO_LED2         CC2650DK_7ID_PIN_LED3
#define Board_GPIO_LED3         CC2650DK_7ID_PIN_LED4
#define Board_GPIO_LED_ON       CC2650DK_7ID_GPIO_LED_ON
#define Board_GPIO_LED_OFF      CC2650DK_7ID_GPIO_LED_OFF

#define Board_GPTIMER0A         CC2650DK_7ID_GPTIMER0A
#define Board_GPTIMER0B         CC2650DK_7ID_GPTIMER0B
#define Board_GPTIMER1A         CC2650DK_7ID_GPTIMER1A
#define Board_GPTIMER1B         CC2650DK_7ID_GPTIMER1B
#define Board_GPTIMER2A         CC2650DK_7ID_GPTIMER2A
#define Board_GPTIMER2B         CC2650DK_7ID_GPTIMER2B
#define Board_GPTIMER3A         CC2650DK_7ID_GPTIMER3A
#define Board_GPTIMER3B         CC2650DK_7ID_GPTIMER3B

#define Board_I2C0              CC2650DK_7ID_I2C0

#define Board_I2S0              CC2650DK_7ID_I2S0
#define Board_I2S_ADO           CC2650DK_7ID_I2S_ADO
#define Board_I2S_ADI           CC2650DK_7ID_I2S_ADI
#define Board_I2S_BCLK          CC2650DK_7ID_I2S_BCLK
#define Board_I2S_MCLK          CC2650DK_7ID_I2S_MCLK
#define Board_I2S_WCLK          CC2650DK_7ID_I2S_WCLK

#define Board_NVSINTERNAL       CC2650DK_7ID_NVSCC26XX0

#define Board_KEY_SELECT        CC2650DK_7ID_PIN_KEY_SELECT
#define Board_KEY_UP            CC2650DK_7ID_PIN_KEY_UP
#define Board_KEY_DOWN          CC2650DK_7ID_PIN_KEY_DOWN
#define Board_KEY_LEFT          CC2650DK_7ID_PIN_KEY_LEFT
#define Board_KEY_RIGHT         CC2650DK_7ID_PIN_KEY_RIGHT

#define Board_PIN_BUTTON0       CC2650DK_7ID_PIN_KEY_SELECT
#define Board_PIN_BUTTON1       CC2650DK_7ID_PIN_KEY_UP
#define Board_PIN_BUTTON2       CC2650DK_7ID_PIN_KEY_DOWN
#define Board_PIN_BUTTON3       CC2650DK_7ID_PIN_KEY_LEFT
#define Board_PIN_BUTTON4       CC2650DK_7ID_PIN_KEY_RIGHT
#define Board_PIN_BTN1          CC2650DK_7ID_PIN_KEY_SELECT
#define Board_PIN_BTN2          CC2650DK_7ID_PIN_KEY_UP
#define Board_PIN_BTN3          CC2650DK_7ID_PIN_KEY_DOWN
#define Board_PIN_BTN4          CC2650DK_7ID_PIN_KEY_LEFT
#define Board_PIN_BTN5          CC2650DK_7ID_PIN_KEY_RIGHT
#define Board_PIN_LED0          CC2650DK_7ID_PIN_LED1
#define Board_PIN_LED1          CC2650DK_7ID_PIN_LED2
#define Board_PIN_LED2          CC2650DK_7ID_PIN_LED3
#define Board_PIN_LED3          CC2650DK_7ID_PIN_LED4

#define Board_PWM0              CC2650DK_7ID_PWM0
#define Board_PWM1              CC2650DK_7ID_PWM1
#define Board_PWM2              CC2650DK_7ID_PWM2
#define Board_PWM3              CC2650DK_7ID_PWM3
#define Board_PWM4              CC2650DK_7ID_PWM4
#define Board_PWM5              CC2650DK_7ID_PWM5
#define Board_PWM6              CC2650DK_7ID_PWM6
#define Board_PWM7              CC2650DK_7ID_PWM7

#define Board_SD0               CC2650DK_7ID_SDSPI0

#define Board_SPI0              CC2650DK_7ID_SPI0
#define Board_SPI0_MISO         CC2650DK_7ID_SPI0_MISO
#define Board_SPI0_MOSI         CC2650DK_7ID_SPI0_MOSI
#define Board_SPI0_CLK          CC2650DK_7ID_SPI0_CLK
#define Board_SPI0_CSN          CC2650DK_7ID_SPI0_CSN
#define Board_SPI1              CC2650DK_7ID_SPI1
#define Board_SPI1_MISO         CC2650DK_7ID_SPI1_MISO
#define Board_SPI1_MOSI         CC2650DK_7ID_SPI1_MOSI
#define Board_SPI1_CLK          CC2650DK_7ID_SPI1_CLK
#define Board_SPI1_CSN          CC2650DK_7ID_SPI1_CSN
#define Board_FLASH_CS_ON       0
#define Board_FLASH_CS_OFF      1

#define Board_SPI_MASTER        CC2650DK_7ID_SPI0
#define Board_SPI_SLAVE         CC2650DK_7ID_SPI0
#define Board_SPI_MASTER_READY  CC2650DK_7ID_SPI_MASTER_READY
#define Board_SPI_SLAVE_READY   CC2650DK_7ID_SPI_SLAVE_READY

#define Board_UART0             CC2650DK_7ID_UART0

#define Board_WATCHDOG0         CC2650DK_7ID_WATCHDOG0

#define Board_SDCARD_CS         CC2650DK_7ID_SDCARD_CS

#define Board_LCD_MODE          CC2650DK_7ID_LCD_MODE
#define Board_LCD_RST           CC2650DK_7ID_LCD_RST
#define Board_LCD_CS            CC2650DK_7ID_LCD_CS

#define Board_ALS_OUT           CC2650DK_7ID_ALS_OUT
#define Board_ALS_PWR           CC2650DK_7ID_ALS_PWR

#define Board_ACC_PWR           CC2650DK_7ID_ACC_PWR
#define Board_ACC_CS            CC2650DK_7ID_ACC_CS

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */

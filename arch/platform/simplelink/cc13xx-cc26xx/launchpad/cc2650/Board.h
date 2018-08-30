/*
 * Copyright (c) 2015-2018, Texas Instruments Incorporated
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

#define Board_CC2650_LAUNCHXL
#define BOARD_STRING    "TI CC2650 LaunchPad"

#ifdef __cplusplus
extern "C" {
#endif

#include "CC2650_LAUNCHXL.h"

#define Board_initGeneral()     CC2650_LAUNCHXL_initGeneral()
#define Board_shutDownExtFlash() CC2650_LAUNCHXL_shutDownExtFlash()
#define Board_wakeUpExtFlash() CC2650_LAUNCHXL_wakeUpExtFlash()

/* These #defines allow us to reuse TI-RTOS across other device families */

#define Board_ADC0              CC2650_LAUNCHXL_ADC0
#define Board_ADC1              CC2650_LAUNCHXL_ADC1

#define Board_ADCBUF0           CC2650_LAUNCHXL_ADCBUF0
#define Board_ADCBUF0CHANNEL0   CC2650_LAUNCHXL_ADCBUF0CHANNEL0
#define Board_ADCBUF0CHANNEL1   CC2650_LAUNCHXL_ADCBUF0CHANNEL1

#define Board_CRYPTO0           CC2650_LAUNCHXL_CRYPTO0

#define Board_DIO0              CC2650_LAUNCHXL_DIO0
#define Board_DIO1_RFSW         CC2650_LAUNCHXL_DIO1_RFSW
#define Board_DIO12             CC2650_LAUNCHXL_DIO12
#define Board_DIO15             CC2650_LAUNCHXL_DIO15
#define Board_DIO16_TDO         CC2650_LAUNCHXL_DIO16_TDO
#define Board_DIO17_TDI         CC2650_LAUNCHXL_DIO17_TDI
#define Board_DIO21             CC2650_LAUNCHXL_DIO21
#define Board_DIO22             CC2650_LAUNCHXL_DIO22

#define Board_DIO23_ANALOG      CC2650_LAUNCHXL_DIO23_ANALOG
#define Board_DIO24_ANALOG      CC2650_LAUNCHXL_DIO24_ANALOG
#define Board_DIO25_ANALOG      CC2650_LAUNCHXL_DIO25_ANALOG
#define Board_DIO26_ANALOG      CC2650_LAUNCHXL_DIO26_ANALOG
#define Board_DIO27_ANALOG      CC2650_LAUNCHXL_DIO27_ANALOG
#define Board_DIO28_ANALOG      CC2650_LAUNCHXL_DIO28_ANALOG
#define Board_DIO29_ANALOG      CC2650_LAUNCHXL_DIO29_ANALOG
#define Board_DIO30_ANALOG      CC2650_LAUNCHXL_DIO30_ANALOG

#define Board_GPIO_BUTTON0      CC2650_LAUNCHXL_GPIO_S1
#define Board_GPIO_BUTTON1      CC2650_LAUNCHXL_GPIO_S2
#define Board_GPIO_BTN1         CC2650_LAUNCHXL_GPIO_S1
#define Board_GPIO_BTN2         CC2650_LAUNCHXL_GPIO_S2
#define Board_GPIO_LED0         CC2650_LAUNCHXL_GPIO_LED_RED
#define Board_GPIO_LED1         CC2650_LAUNCHXL_GPIO_LED_GREEN
#define Board_GPIO_RLED         CC2650_LAUNCHXL_GPIO_LED_RED
#define Board_GPIO_GLED         CC2650_LAUNCHXL_GPIO_LED_GREEN
#define Board_GPIO_LED_ON       CC2650_LAUNCHXL_GPIO_LED_ON
#define Board_GPIO_LED_OFF      CC2650_LAUNCHXL_GPIO_LED_OFF

#define Board_GPTIMER0A         CC2650_LAUNCHXL_GPTIMER0A
#define Board_GPTIMER0B         CC2650_LAUNCHXL_GPTIMER0B
#define Board_GPTIMER1A         CC2650_LAUNCHXL_GPTIMER1A
#define Board_GPTIMER1B         CC2650_LAUNCHXL_GPTIMER1B
#define Board_GPTIMER2A         CC2650_LAUNCHXL_GPTIMER2A
#define Board_GPTIMER2B         CC2650_LAUNCHXL_GPTIMER2B
#define Board_GPTIMER3A         CC2650_LAUNCHXL_GPTIMER3A
#define Board_GPTIMER3B         CC2650_LAUNCHXL_GPTIMER3B

#define Board_I2C0              CC2650_LAUNCHXL_I2C0
#define Board_I2C_TMP           CC2650_LAUNCHXL_I2C0

#define Board_NVSINTERNAL       CC2650_LAUNCHXL_NVSCC26XX0
#define Board_NVSEXTERNAL       CC2650_LAUNCHXL_NVSSPI25X0

#define Board_PIN_BUTTON0       CC2650_LAUNCHXL_PIN_BTN1
#define Board_PIN_BUTTON1       CC2650_LAUNCHXL_PIN_BTN2
#define Board_PIN_BTN1          CC2650_LAUNCHXL_PIN_BTN1
#define Board_PIN_BTN2          CC2650_LAUNCHXL_PIN_BTN2
#define Board_PIN_LED0          CC2650_LAUNCHXL_PIN_RLED
#define Board_PIN_LED1          CC2650_LAUNCHXL_PIN_GLED
#define Board_PIN_LED2          CC2650_LAUNCHXL_PIN_RLED
#define Board_PIN_RLED          CC2650_LAUNCHXL_PIN_RLED
#define Board_PIN_GLED          CC2650_LAUNCHXL_PIN_GLED

#define Board_PWM0              CC2650_LAUNCHXL_PWM0
#define Board_PWM1              CC2650_LAUNCHXL_PWM1
#define Board_PWM2              CC2650_LAUNCHXL_PWM2
#define Board_PWM3              CC2650_LAUNCHXL_PWM3
#define Board_PWM4              CC2650_LAUNCHXL_PWM4
#define Board_PWM5              CC2650_LAUNCHXL_PWM5
#define Board_PWM6              CC2650_LAUNCHXL_PWM6
#define Board_PWM7              CC2650_LAUNCHXL_PWM7

#define Board_SD0               CC2650_LAUNCHXL_SDSPI0

#define Board_SPI0              CC2650_LAUNCHXL_SPI0
#define Board_SPI0_MISO         CC2650_LAUNCHXL_SPI0_MISO
#define Board_SPI0_MOSI         CC2650_LAUNCHXL_SPI0_MOSI
#define Board_SPI0_CLK          CC2650_LAUNCHXL_SPI0_CLK
#define Board_SPI0_CSN          CC2650_LAUNCHXL_SPI0_CSN
#define Board_SPI1              CC2650_LAUNCHXL_SPI1
#define Board_SPI1_MISO         CC2650_LAUNCHXL_SPI1_MISO
#define Board_SPI1_MOSI         CC2650_LAUNCHXL_SPI1_MOSI
#define Board_SPI1_CLK          CC2650_LAUNCHXL_SPI1_CLK
#define Board_SPI1_CSN          CC2650_LAUNCHXL_SPI1_CSN
#define Board_SPI_FLASH_CS      CC2650_LAUNCHXL_SPI_FLASH_CS
#define Board_FLASH_CS_ON       0
#define Board_FLASH_CS_OFF      1

#define Board_SPI_MASTER        CC2650_LAUNCHXL_SPI0
#define Board_SPI_SLAVE         CC2650_LAUNCHXL_SPI0
#define Board_SPI_MASTER_READY  CC2650_LAUNCHXL_SPI_MASTER_READY
#define Board_SPI_SLAVE_READY   CC2650_LAUNCHXL_SPI_SLAVE_READY

#define Board_UART0             CC2650_LAUNCHXL_UART0

#define Board_WATCHDOG0         CC2650_LAUNCHXL_WATCHDOG0

/* Board specific I2C addresses */
#define Board_TMP_ADDR          (0x40)
#define Board_SENSORS_BP_TMP_ADDR Board_TMP_ADDR

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */

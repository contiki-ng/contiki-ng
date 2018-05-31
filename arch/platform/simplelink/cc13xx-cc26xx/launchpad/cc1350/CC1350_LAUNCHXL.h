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
/** ============================================================================
 *  @file       CC1350_LAUNCHXL.h
 *
 *  @brief      CC1350 LaunchPad Board Specific header file.
 *
 *  The CC1350_LAUNCHXL header file should be included in an application as
 *  follows:
 *  @code
 *  #include "CC1350_LAUNCHXL.h"
 *  @endcode
 *
 *  ============================================================================
 */
#ifndef __CC1350_LAUNCHXL_BOARD_H__
#define __CC1350_LAUNCHXL_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include <ti/drivers/PIN.h>
#include <ti/devices/cc13x0/driverlib/ioc.h>

/* Externs */
extern const PIN_Config BoardGpioInitTable[];

/* Defines */
#define CC1350_LAUNCHXL

/* Mapping of pins to board signals using general board aliases
 *      <board signal alias>        <pin mapping>   <comments>
 */

/* Analog Capable DIOs */
#define CC1350_LAUNCHXL_DIO23_ANALOG          IOID_23
#define CC1350_LAUNCHXL_DIO24_ANALOG          IOID_24
#define CC1350_LAUNCHXL_DIO25_ANALOG          IOID_25
#define CC1350_LAUNCHXL_DIO26_ANALOG          IOID_26
#define CC1350_LAUNCHXL_DIO27_ANALOG          IOID_27
#define CC1350_LAUNCHXL_DIO28_ANALOG          IOID_28
#define CC1350_LAUNCHXL_DIO29_ANALOG          IOID_29
#define CC1350_LAUNCHXL_DIO30_ANALOG          IOID_30

/* Digital IOs */
#define CC1350_LAUNCHXL_DIO0                  IOID_0
#define CC1350_LAUNCHXL_DIO1_RF_SUB1GHZ       IOID_1
#define CC1350_LAUNCHXL_DIO12                 IOID_12
#define CC1350_LAUNCHXL_DIO15                 IOID_15
#define CC1350_LAUNCHXL_DIO16_TDO             IOID_16
#define CC1350_LAUNCHXL_DIO17_TDI             IOID_17
#define CC1350_LAUNCHXL_DIO21                 IOID_21
#define CC1350_LAUNCHXL_DIO22                 IOID_22
#define CC1350_LAUNCHXL_DIO30_RF_POWER        IOID_30

/* Discrete Inputs */
#define CC1350_LAUNCHXL_PIN_BTN1              IOID_13
#define CC1350_LAUNCHXL_PIN_BTN2              IOID_14


/* GPIO */
#define CC1350_LAUNCHXL_GPIO_LED_ON           1
#define CC1350_LAUNCHXL_GPIO_LED_OFF          0

/* I2C */
#define CC1350_LAUNCHXL_I2C0_SCL0             IOID_4
#define CC1350_LAUNCHXL_I2C0_SDA0             IOID_5


/* LEDs */
#define CC1350_LAUNCHXL_PIN_LED_ON            1
#define CC1350_LAUNCHXL_PIN_LED_OFF           0
#define CC1350_LAUNCHXL_PIN_RLED              IOID_6
#define CC1350_LAUNCHXL_PIN_GLED              IOID_7

/* PWM Outputs */
#define CC1350_LAUNCHXL_PWMPIN0               CC1350_LAUNCHXL_PIN_RLED
#define CC1350_LAUNCHXL_PWMPIN1               CC1350_LAUNCHXL_PIN_GLED
#define CC1350_LAUNCHXL_PWMPIN2               PIN_UNASSIGNED
#define CC1350_LAUNCHXL_PWMPIN3               PIN_UNASSIGNED
#define CC1350_LAUNCHXL_PWMPIN4               PIN_UNASSIGNED
#define CC1350_LAUNCHXL_PWMPIN5               PIN_UNASSIGNED
#define CC1350_LAUNCHXL_PWMPIN6               PIN_UNASSIGNED
#define CC1350_LAUNCHXL_PWMPIN7               PIN_UNASSIGNED

/* SPI */
#define CC1350_LAUNCHXL_SPI_FLASH_CS          IOID_20
#define CC1350_LAUNCHXL_FLASH_CS_ON           0
#define CC1350_LAUNCHXL_FLASH_CS_OFF          1

/* SPI Board */
#define CC1350_LAUNCHXL_SPI0_MISO             IOID_8          /* RF1.20 */
#define CC1350_LAUNCHXL_SPI0_MOSI             IOID_9          /* RF1.18 */
#define CC1350_LAUNCHXL_SPI0_CLK              IOID_10         /* RF1.16 */
#define CC1350_LAUNCHXL_SPI0_CSN              PIN_UNASSIGNED
#define CC1350_LAUNCHXL_SPI1_MISO             PIN_UNASSIGNED
#define CC1350_LAUNCHXL_SPI1_MOSI             PIN_UNASSIGNED
#define CC1350_LAUNCHXL_SPI1_CLK              PIN_UNASSIGNED
#define CC1350_LAUNCHXL_SPI1_CSN              PIN_UNASSIGNED

/* UART Board */
#define CC1350_LAUNCHXL_UART_RX               IOID_2          /* RXD */
#define CC1350_LAUNCHXL_UART_TX               IOID_3          /* TXD */
#define CC1350_LAUNCHXL_UART_CTS              IOID_19         /* CTS */
#define CC1350_LAUNCHXL_UART_RTS              IOID_18         /* RTS */

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
void CC1350_LAUNCHXL_initGeneral(void);

/*!
 *  @brief  Turn off the external flash on LaunchPads
 *
 */
void CC1350_LAUNCHXL_shutDownExtFlash(void);

/*!
 *  @brief  Wake up the external flash present on the board files
 *
 *  This function toggles the chip select for the amount of time needed
 *  to wake the chip up.
 */
void CC1350_LAUNCHXL_wakeUpExtFlash(void);

/*!
 *  @def    CC1350_LAUNCHXL_ADCBufName
 *  @brief  Enum of ADCBufs
 */
typedef enum CC1350_LAUNCHXL_ADCBufName {
    CC1350_LAUNCHXL_ADCBUF0 = 0,

    CC1350_LAUNCHXL_ADCBUFCOUNT
} CC1350_LAUNCHXL_ADCBufName;

/*!
 *  @def    CC1350_LAUNCHXL_ADCBuf0SourceName
 *  @brief  Enum of ADCBuf channels
 */
typedef enum CC1350_LAUNCHXL_ADCBuf0ChannelName {
    CC1350_LAUNCHXL_ADCBUF0CHANNEL0 = 0,
    CC1350_LAUNCHXL_ADCBUF0CHANNEL1,
    CC1350_LAUNCHXL_ADCBUF0CHANNEL2,
    CC1350_LAUNCHXL_ADCBUF0CHANNEL3,
    CC1350_LAUNCHXL_ADCBUF0CHANNEL4,
    CC1350_LAUNCHXL_ADCBUF0CHANNEL5,
    CC1350_LAUNCHXL_ADCBUF0CHANNEL6,
    CC1350_LAUNCHXL_ADCBUF0CHANNEL7,
    CC1350_LAUNCHXL_ADCBUF0CHANNELVDDS,
    CC1350_LAUNCHXL_ADCBUF0CHANNELDCOUPL,
    CC1350_LAUNCHXL_ADCBUF0CHANNELVSS,

    CC1350_LAUNCHXL_ADCBUF0CHANNELCOUNT
} CC1350_LAUNCHXL_ADCBuf0ChannelName;

/*!
 *  @def    CC1350_LAUNCHXL_ADCName
 *  @brief  Enum of ADCs
 */
typedef enum CC1350_LAUNCHXL_ADCName {
    CC1350_LAUNCHXL_ADC0 = 0,
    CC1350_LAUNCHXL_ADC1,
    CC1350_LAUNCHXL_ADC2,
    CC1350_LAUNCHXL_ADC3,
    CC1350_LAUNCHXL_ADC4,
    CC1350_LAUNCHXL_ADC5,
    CC1350_LAUNCHXL_ADC6,
    CC1350_LAUNCHXL_ADC7,
    CC1350_LAUNCHXL_ADCDCOUPL,
    CC1350_LAUNCHXL_ADCVSS,
    CC1350_LAUNCHXL_ADCVDDS,

    CC1350_LAUNCHXL_ADCCOUNT
} CC1350_LAUNCHXL_ADCName;

/*!
 *  @def    CC1350_LAUNCHXL_CryptoName
 *  @brief  Enum of Crypto names
 */
typedef enum CC1350_LAUNCHXL_CryptoName {
    CC1350_LAUNCHXL_CRYPTO0 = 0,

    CC1350_LAUNCHXL_CRYPTOCOUNT
} CC1350_LAUNCHXL_CryptoName;

/*!
 *  @def    CC1350_LAUNCHXL_GPIOName
 *  @brief  Enum of GPIO names
 */
typedef enum CC1350_LAUNCHXL_GPIOName {
    CC1350_LAUNCHXL_GPIO_S1 = 0,
    CC1350_LAUNCHXL_GPIO_S2,
    CC1350_LAUNCHXL_SPI_MASTER_READY,
    CC1350_LAUNCHXL_SPI_SLAVE_READY,
    CC1350_LAUNCHXL_GPIO_LED_GREEN,
    CC1350_LAUNCHXL_GPIO_LED_RED,
    CC1350_LAUNCHXL_GPIO_SPI_FLASH_CS,
    CC1350_LAUNCHXL_SDSPI_CS,
    CC1350_LAUNCHXL_GPIO_LCD_CS,
    CC1350_LAUNCHXL_GPIO_LCD_POWER,
    CC1350_LAUNCHXL_GPIO_LCD_ENABLE,
    CC1350_LAUNCHXL_GPIOCOUNT
} CC1350_LAUNCHXL_GPIOName;

/*!
 *  @def    CC1350_LAUNCHXL_GPTimerName
 *  @brief  Enum of GPTimer parts
 */
typedef enum CC1350_LAUNCHXL_GPTimerName {
    CC1350_LAUNCHXL_GPTIMER0A = 0,
    CC1350_LAUNCHXL_GPTIMER0B,
    CC1350_LAUNCHXL_GPTIMER1A,
    CC1350_LAUNCHXL_GPTIMER1B,
    CC1350_LAUNCHXL_GPTIMER2A,
    CC1350_LAUNCHXL_GPTIMER2B,
    CC1350_LAUNCHXL_GPTIMER3A,
    CC1350_LAUNCHXL_GPTIMER3B,

    CC1350_LAUNCHXL_GPTIMERPARTSCOUNT
} CC1350_LAUNCHXL_GPTimerName;

/*!
 *  @def    CC1350_LAUNCHXL_GPTimers
 *  @brief  Enum of GPTimers
 */
typedef enum CC1350_LAUNCHXL_GPTimers {
    CC1350_LAUNCHXL_GPTIMER0 = 0,
    CC1350_LAUNCHXL_GPTIMER1,
    CC1350_LAUNCHXL_GPTIMER2,
    CC1350_LAUNCHXL_GPTIMER3,

    CC1350_LAUNCHXL_GPTIMERCOUNT
} CC1350_LAUNCHXL_GPTimers;

/*!
 *  @def    CC1350_LAUNCHXL_I2CName
 *  @brief  Enum of I2C names
 */
typedef enum CC1350_LAUNCHXL_I2CName {
    CC1350_LAUNCHXL_I2C0 = 0,

    CC1350_LAUNCHXL_I2CCOUNT
} CC1350_LAUNCHXL_I2CName;

/*!
 *  @def    CC1350_LAUNCHXL_NVSName
 *  @brief  Enum of NVS names
 */
typedef enum CC1350_LAUNCHXL_NVSName {
#ifndef Board_EXCLUDE_NVS_INTERNAL_FLASH
    CC1350_LAUNCHXL_NVSCC26XX0 = 0,
#endif
#ifndef Board_EXCLUDE_NVS_EXTERNAL_FLASH
    CC1350_LAUNCHXL_NVSSPI25X0,
#endif

    CC1350_LAUNCHXL_NVSCOUNT
} CC1350_LAUNCHXL_NVSName;

/*!
 *  @def    CC1350_LAUNCHXL_PWM
 *  @brief  Enum of PWM outputs
 */
typedef enum CC1350_LAUNCHXL_PWMName {
    CC1350_LAUNCHXL_PWM0 = 0,
    CC1350_LAUNCHXL_PWM1,
    CC1350_LAUNCHXL_PWM2,
    CC1350_LAUNCHXL_PWM3,
    CC1350_LAUNCHXL_PWM4,
    CC1350_LAUNCHXL_PWM5,
    CC1350_LAUNCHXL_PWM6,
    CC1350_LAUNCHXL_PWM7,

    CC1350_LAUNCHXL_PWMCOUNT
} CC1350_LAUNCHXL_PWMName;

/*!
 *  @def    CC1350_LAUNCHXL_SDName
 *  @brief  Enum of SD names
 */
typedef enum CC1350_LAUNCHXL_SDName {
    CC1350_LAUNCHXL_SDSPI0 = 0,

    CC1350_LAUNCHXL_SDCOUNT
} CC1350_LAUNCHXL_SDName;

/*!
 *  @def    CC1350_LAUNCHXL_SPIName
 *  @brief  Enum of SPI names
 */
typedef enum CC1350_LAUNCHXL_SPIName {
    CC1350_LAUNCHXL_SPI0 = 0,
    CC1350_LAUNCHXL_SPI1,

    CC1350_LAUNCHXL_SPICOUNT
} CC1350_LAUNCHXL_SPIName;

/*!
 *  @def    CC1350_LAUNCHXL_UARTName
 *  @brief  Enum of UARTs
 */
typedef enum CC1350_LAUNCHXL_UARTName {
    CC1350_LAUNCHXL_UART0 = 0,

    CC1350_LAUNCHXL_UARTCOUNT
} CC1350_LAUNCHXL_UARTName;

/*!
 *  @def    CC1350_LAUNCHXL_UDMAName
 *  @brief  Enum of DMA buffers
 */
typedef enum CC1350_LAUNCHXL_UDMAName {
    CC1350_LAUNCHXL_UDMA0 = 0,

    CC1350_LAUNCHXL_UDMACOUNT
} CC1350_LAUNCHXL_UDMAName;

/*!
 *  @def    CC1350_LAUNCHXL_WatchdogName
 *  @brief  Enum of Watchdogs
 */
typedef enum CC1350_LAUNCHXL_WatchdogName {
    CC1350_LAUNCHXL_WATCHDOG0 = 0,

    CC1350_LAUNCHXL_WATCHDOGCOUNT
} CC1350_LAUNCHXL_WatchdogName;

#ifdef __cplusplus
}
#endif

#endif /* __CC1350_LAUNCHXL_BOARD_H__ */

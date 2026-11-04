/*
 * Copyright (c) 2015-2019, Texas Instruments Incorporated
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
 *  @file       LP_CC1312R7.h
 *
 *  @brief      CC1312R7 LaunchPad Board Specific header file.
 *
 *  The LP_CC1312R7 header file should be included in an application as
 *  follows:
 *  @code
 *  #include "LP_CC1312R7.h"
 *  @endcode
 *
 *  ============================================================================
 */
#ifndef __LP_CC1312R7_BOARD_H__
#define __LP_CC1312R7_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "contiki-conf.h"

/* Includes */
#include <ti/drivers/PIN.h>
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/ioc.h)

/* Externs */
extern const PIN_Config BoardGpioInitTable[];

/* Defines */
#define LP_CC1312R7

/* Mapping of pins to board signals using general board aliases
 *      <board signal alias>                  <pin mapping>   <comments>
 */

/* Analog capable DIOs */
#define LP_CC1312R7_DIO23_ANALOG          IOID_23
#define LP_CC1312R7_DIO24_ANALOG          IOID_24
#define LP_CC1312R7_DIO25_ANALOG          IOID_25
#define LP_CC1312R7_DIO26_ANALOG          IOID_26
#define LP_CC1312R7_DIO27_ANALOG          IOID_27
#define LP_CC1312R7_DIO28_ANALOG          IOID_28
#define LP_CC1312R7_DIO29_ANALOG          IOID_29
#define LP_CC1312R7_DIO30_ANALOG          IOID_30

/* Digital IOs */
#define LP_CC1312R7_DIO0                  IOID_0
#define LP_CC1312R7_DIO1                  IOID_1
#define LP_CC1312R7_DIO12                 IOID_12
#define LP_CC1312R7_DIO15                 IOID_15
#define LP_CC1312R7_DIO16_TDO             IOID_16
#define LP_CC1312R7_DIO17_TDI             IOID_17
#define LP_CC1312R7_DIO21                 IOID_21
#define LP_CC1312R7_DIO22                 IOID_22

/* Discrete Inputs */
#define LP_CC1312R7_PIN_BTN1              IOID_13
#define LP_CC1312R7_PIN_BTN2              IOID_14

/* GPIO */
#define LP_CC1312R7_GPIO_LED_ON           1
#define LP_CC1312R7_GPIO_LED_OFF          0

/* I2C */
#define LP_CC1312R7_I2C0_SCL0             IOID_4
#define LP_CC1312R7_I2C0_SDA0             IOID_5

/* I2S */
#define LP_CC1312R7_I2S_ADO               IOID_25
#define LP_CC1312R7_I2S_ADI               IOID_26
#define LP_CC1312R7_I2S_BCLK              IOID_27
#define LP_CC1312R7_I2S_MCLK              PIN_UNASSIGNED
#define LP_CC1312R7_I2S_WCLK              IOID_28

/* LEDs */
#define LP_CC1312R7_PIN_LED_ON            1
#define LP_CC1312R7_PIN_LED_OFF           0
#define LP_CC1312R7_PIN_RLED              IOID_6
#define LP_CC1312R7_PIN_GLED              IOID_7

/* PWM Outputs */
#define LP_CC1312R7_PWMPIN0               LP_CC1312R7_PIN_RLED
#define LP_CC1312R7_PWMPIN1               LP_CC1312R7_PIN_GLED
#define LP_CC1312R7_PWMPIN2               PIN_UNASSIGNED
#define LP_CC1312R7_PWMPIN3               PIN_UNASSIGNED
#define LP_CC1312R7_PWMPIN4               PIN_UNASSIGNED
#define LP_CC1312R7_PWMPIN5               PIN_UNASSIGNED
#define LP_CC1312R7_PWMPIN6               PIN_UNASSIGNED
#define LP_CC1312R7_PWMPIN7               PIN_UNASSIGNED

/* SPI */
#define LP_CC1312R7_SPI_FLASH_CS          IOID_20
#define LP_CC1312R7_FLASH_CS_ON           0
#define LP_CC1312R7_FLASH_CS_OFF          1

/* SPI Board */
#define LP_CC1312R7_SPI0_MISO             IOID_8          /* RF1.20 */
#define LP_CC1312R7_SPI0_MOSI             IOID_9          /* RF1.18 */
#define LP_CC1312R7_SPI0_CLK              IOID_10         /* RF1.16 */
#define LP_CC1312R7_SPI0_CSN              IOID_11
#define LP_CC1312R7_SPI1_MISO             PIN_UNASSIGNED
#define LP_CC1312R7_SPI1_MOSI             PIN_UNASSIGNED
#define LP_CC1312R7_SPI1_CLK              PIN_UNASSIGNED
#define LP_CC1312R7_SPI1_CSN              PIN_UNASSIGNED

/* UART Board */
#define LP_CC1312R7_UART0_RX              IOID_2          /* RXD */
#define LP_CC1312R7_UART0_TX              IOID_3          /* TXD */
#define LP_CC1312R7_UART0_CTS             IOID_19         /* CTS */
#define LP_CC1312R7_UART0_RTS             IOID_18         /* RTS */
#define LP_CC1312R7_UART1_RX              PIN_UNASSIGNED
#define LP_CC1312R7_UART1_TX              PIN_UNASSIGNED
#define LP_CC1312R7_UART1_CTS             PIN_UNASSIGNED
#define LP_CC1312R7_UART1_RTS             PIN_UNASSIGNED
/* For backward compatibility */
#define LP_CC1312R7_UART_RX               LP_CC1312R7_UART0_RX
#define LP_CC1312R7_UART_TX               LP_CC1312R7_UART0_TX
#define LP_CC1312R7_UART_CTS              LP_CC1312R7_UART0_CTS
#define LP_CC1312R7_UART_RTS              LP_CC1312R7_UART0_RTS

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
void LP_CC1312R7_initGeneral(void);

/*!
 *  @brief  Turn off the external flash on LaunchPads
 *
 */
void LP_CC1312R7_shutDownExtFlash(void);

/*!
 *  @brief  Wake up the external flash present on the board files
 *
 *  This function toggles the chip select for the amount of time needed
 *  to wake the chip up.
 */
void LP_CC1312R7_wakeUpExtFlash(void);

/*!
 *  @def    LP_CC1312R7_ADCBufName
 *  @brief  Enum of ADCBufs
 */
typedef enum LP_CC1312R7_ADCBufName {
    LP_CC1312R7_ADCBUF0 = 0,

    LP_CC1312R7_ADCBUFCOUNT
} LP_CC1312R7_ADCBufName;

/*!
 *  @def    LP_CC1312R7_ADCBuf0ChannelName
 *  @brief  Enum of ADCBuf channels
 */
typedef enum LP_CC1312R7_ADCBuf0ChannelName {
    LP_CC1312R7_ADCBUF0CHANNEL0 = 0,
    LP_CC1312R7_ADCBUF0CHANNEL1,
    LP_CC1312R7_ADCBUF0CHANNEL2,
    LP_CC1312R7_ADCBUF0CHANNEL3,
    LP_CC1312R7_ADCBUF0CHANNEL4,
    LP_CC1312R7_ADCBUF0CHANNEL5,
    LP_CC1312R7_ADCBUF0CHANNEL6,
    LP_CC1312R7_ADCBUF0CHANNEL7,
    LP_CC1312R7_ADCBUF0CHANNELVDDS,
    LP_CC1312R7_ADCBUF0CHANNELDCOUPL,
    LP_CC1312R7_ADCBUF0CHANNELVSS,

    LP_CC1312R7_ADCBUF0CHANNELCOUNT
} LP_CC1312R7_ADCBuf0ChannelName;

/*!
 *  @def    LP_CC1312R7_ADCName
 *  @brief  Enum of ADCs
 */
typedef enum LP_CC1312R7_ADCName {
    LP_CC1312R7_ADC0 = 0,
    LP_CC1312R7_ADC1,
    LP_CC1312R7_ADC2,
    LP_CC1312R7_ADC3,
    LP_CC1312R7_ADC4,
    LP_CC1312R7_ADC5,
    LP_CC1312R7_ADC6,
    LP_CC1312R7_ADC7,
    LP_CC1312R7_ADCDCOUPL,
    LP_CC1312R7_ADCVSS,
    LP_CC1312R7_ADCVDDS,

    LP_CC1312R7_ADCCOUNT
} LP_CC1312R7_ADCName;

/*!
 *  @def    LP_CC1312R7_ECDHName
 *  @brief  Enum of ECDH names
 */
typedef enum LP_CC1312R7_ECDHName {
    LP_CC1312R7_ECDH0 = 0,

    LP_CC1312R7_ECDHCOUNT
} LP_CC1312R7_ECDHName;

/*!
 *  @def    LP_CC1312R7_ECDSAName
 *  @brief  Enum of ECDSA names
 */
typedef enum LP_CC1312R7_ECDSAName {
    LP_CC1312R7_ECDSA0 = 0,

    LP_CC1312R7_ECDSACOUNT
} LP_CC1312R7_ECDSAName;

/*!
 *  @def    LP_CC1312R7_ECJPAKEName
 *  @brief  Enum of ECJPAKE names
 */
typedef enum LP_CC1312R7_ECJPAKEName {
    LP_CC1312R7_ECJPAKE0 = 0,

    LP_CC1312R7_ECJPAKECOUNT
} LP_CC1312R7_ECJPAKEName;

/*!
 *  @def    LP_CC1312R7_AESCCMName
 *  @brief  Enum of AESCCM names
 */
typedef enum LP_CC1312R7_AESCCMName {
    LP_CC1312R7_AESCCM0 = 0,

    LP_CC1312R7_AESCCMCOUNT
} LP_CC1312R7_AESCCMName;

/*!
 *  @def    LP_CC1312R7_AESGCMName
 *  @brief  Enum of AESGCM names
 */
typedef enum LP_CC1312R7_AESGCMName {
    LP_CC1312R7_AESGCM0 = 0,

    LP_CC1312R7_AESGCMCOUNT
} LP_CC1312R7_AESGCMName;

/*!
 *  @def    LP_CC1312R7_AESCBCName
 *  @brief  Enum of AESCBC names
 */
typedef enum LP_CC1312R7_AESCBCName {
    LP_CC1312R7_AESCBC0 = 0,

    LP_CC1312R7_AESCBCCOUNT
} LP_CC1312R7_AESCBCName;

/*!
 *  @def    LP_CC1312R7_AESCTRName
 *  @brief  Enum of AESCTR names
 */
typedef enum LP_CC1312R7_AESCTRName {
    LP_CC1312R7_AESCTR0 = 0,

    LP_CC1312R7_AESCTRCOUNT
} LP_CC1312R7_AESCTRName;

/*!
 *  @def    LP_CC1312R7_AESECBName
 *  @brief  Enum of AESECB names
 */
typedef enum LP_CC1312R7_AESECBName {
    LP_CC1312R7_AESECB0 = 0,

    LP_CC1312R7_AESECBCOUNT
} LP_CC1312R7_AESECBName;

/*!
 *  @def    LP_CC1312R7_AESCTRDRBGName
 *  @brief  Enum of AESCTRDRBG names
 */
typedef enum LP_CC1312R7_AESCTRDRBGName {
    LP_CC1312R7_AESCTRDRBG0 = 0,

    LP_CC1312R7_AESCTRDRBGCOUNT
} LP_CC1312R7_AESCTRDRBGName;

/*!
 *  @def    LP_CC1312R7_SHA2Name
 *  @brief  Enum of SHA2 names
 */
typedef enum LP_CC1312R7_SHA2Name {
    LP_CC1312R7_SHA20 = 0,

    LP_CC1312R7_SHA2COUNT
} LP_CC1312R7_SHA2Name;

/*!
 *  @def    LP_CC1312R7_TRNGName
 *  @brief  Enum of TRNG names
 */
typedef enum LP_CC1312R7_TRNGName {
    LP_CC1312R7_TRNG0 = 0,

    LP_CC1312R7_TRNGCOUNT
} LP_CC1312R7_TRNGName;

/*!
 *  @def    LP_CC1312R7_GPIOName
 *  @brief  Enum of GPIO names
 */
typedef enum LP_CC1312R7_GPIOName {
    LP_CC1312R7_GPIO_S1 = 0,
    LP_CC1312R7_GPIO_S2,
    LP_CC1312R7_SPI_MASTER_READY,
    LP_CC1312R7_SPI_SLAVE_READY,
    LP_CC1312R7_GPIO_LED_GREEN,
    LP_CC1312R7_GPIO_LED_RED,
    LP_CC1312R7_GPIO_SPI_FLASH_CS,
    LP_CC1312R7_SDSPI_CS,
    LP_CC1312R7_GPIOCOUNT
} LP_CC1312R7_GPIOName;

/*!
 *  @def    LP_CC1312R7_GPTimerName
 *  @brief  Enum of GPTimer parts
 */
typedef enum LP_CC1312R7_GPTimerName {
    LP_CC1312R7_GPTIMER0A = 0,
    LP_CC1312R7_GPTIMER0B,
    LP_CC1312R7_GPTIMER1A,
    LP_CC1312R7_GPTIMER1B,
    LP_CC1312R7_GPTIMER2A,
    LP_CC1312R7_GPTIMER2B,
    LP_CC1312R7_GPTIMER3A,
    LP_CC1312R7_GPTIMER3B,

    LP_CC1312R7_GPTIMERPARTSCOUNT
} LP_CC1312R7_GPTimerName;

/*!
 *  @def    LP_CC1312R7_GPTimers
 *  @brief  Enum of GPTimers
 */
typedef enum LP_CC1312R7_GPTimers {
    LP_CC1312R7_GPTIMER0 = 0,
    LP_CC1312R7_GPTIMER1,
    LP_CC1312R7_GPTIMER2,
    LP_CC1312R7_GPTIMER3,

    LP_CC1312R7_GPTIMERCOUNT
} LP_CC1312R7_GPTimers;

/*!
 *  @def    LP_CC1312R7_I2CName
 *  @brief  Enum of I2C names
 */
typedef enum LP_CC1312R7_I2CName {
#if TI_I2C_CONF_I2C0_ENABLE
    LP_CC1312R7_I2C0 = 0,
#endif

    LP_CC1312R7_I2CCOUNT
} LP_CC1312R7_I2CName;

/*!
 *  @def    LP_CC1312R7_I2SName
 *  @brief  Enum of I2S names
 */
typedef enum LP_CC1312R7_I2SName {
    LP_CC1312R7_I2S0 = 0,

    LP_CC1312R7_I2SCOUNT
} LP_CC1312R7_I2SName;

/*!
 *  @def    LP_CC1312R7_NVSName
 *  @brief  Enum of NVS names
 */
typedef enum LP_CC1312R7_NVSName {
#if TI_NVS_CONF_NVS_INTERNAL_ENABLE
    LP_CC1312R7_NVSCC26XX0 = 0,
#endif
#if TI_NVS_CONF_NVS_EXTERNAL_ENABLE
    LP_CC1312R7_NVSSPI25X0,
#endif

    LP_CC1312R7_NVSCOUNT
} LP_CC1312R7_NVSName;

/*!
 *  @def    LP_CC1312R7_PWMName
 *  @brief  Enum of PWM outputs
 */
typedef enum LP_CC1312R7_PWMName {
    LP_CC1312R7_PWM0 = 0,
    LP_CC1312R7_PWM1,
    LP_CC1312R7_PWM2,
    LP_CC1312R7_PWM3,
    LP_CC1312R7_PWM4,
    LP_CC1312R7_PWM5,
    LP_CC1312R7_PWM6,
    LP_CC1312R7_PWM7,

    LP_CC1312R7_PWMCOUNT
} LP_CC1312R7_PWMName;

/*!
 *  @def    LP_CC1312R7_SDName
 *  @brief  Enum of SD names
 */
typedef enum LP_CC1312R7_SDName {
    LP_CC1312R7_SDSPI0 = 0,

    LP_CC1312R7_SDCOUNT
} LP_CC1312R7_SDName;

/*!
 *  @def    LP_CC1312R7_SPIName
 *  @brief  Enum of SPI names
 */
typedef enum LP_CC1312R7_SPIName {
#if TI_SPI_CONF_SPI0_ENABLE
    LP_CC1312R7_SPI0 = 0,
#endif
#if TI_SPI_CONF_SPI1_ENABLE
    LP_CC1312R7_SPI1,
#endif

    LP_CC1312R7_SPICOUNT
} LP_CC1312R7_SPIName;

/*!
 *  @def    LP_CC1312R7_UARTName
 *  @brief  Enum of UARTs
 */
typedef enum LP_CC1312R7_UARTName {
#if TI_UART_CONF_UART0_ENABLE
    LP_CC1312R7_UART0 = 0,
#endif
#if TI_UART_CONF_UART1_ENABLE
    LP_CC1312R7_UART1,
#endif

    LP_CC1312R7_UARTCOUNT
} LP_CC1312R7_UARTName;

/*!
 *  @def    LP_CC1312R7_UDMAName
 *  @brief  Enum of DMA buffers
 */
typedef enum LP_CC1312R7_UDMAName {
    LP_CC1312R7_UDMA0 = 0,

    LP_CC1312R7_UDMACOUNT
} LP_CC1312R7_UDMAName;

/*!
 *  @def    LP_CC1312R7_WatchdogName
 *  @brief  Enum of Watchdogs
 */
typedef enum LP_CC1312R7_WatchdogName {
    LP_CC1312R7_WATCHDOG0 = 0,

    LP_CC1312R7_WATCHDOGCOUNT
} LP_CC1312R7_WatchdogName;


#ifdef __cplusplus
}
#endif

#endif /* __LP_CC1312R7_BOARD_H__ */

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
/** ============================================================================
 *  @file       CC2650STK.h
 *
 *  @brief      CC2650 SensorTag Board Specific header file.
 *
 *  The CC2650STK header file should be included in an application as
 *  follows:
 *  @code
 *  #include "CC2650STK.h"
 *  @endcode
 *  ============================================================================
 */
#ifndef __CC2650STK_BOARD_H__
#define __CC2650STK_BOARD_H__

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
#define CC2650STK

/* Mapping of pins to board signals using general board aliases
 *      <board signal alias>        <pin mapping>   <comments>
 */

/* Analog Capable DIOs */
#define CC2650STK_DIO23_ANALOG          IOID_23
#define CC2650STK_DIO24_ANALOG          IOID_24
#define CC2650STK_DIO25_ANALOG          IOID_25
#define CC2650STK_DIO26_ANALOG          IOID_26
#define CC2650STK_DIO27_ANALOG          IOID_27
#define CC2650STK_DIO28_ANALOG          IOID_28
#define CC2650STK_DIO29_ANALOG          IOID_29
#define CC2650STK_DIO30_ANALOG          IOID_30

/* Audio */
#define CC2650STK_MIC_POWER             IOID_13
#define CC2650STK_MIC_POWER_ON          1
#define CC2650STK_MIC_POWER_OFF         0

/* Buzzer */
#define CC2650STK_BUZZER                IOID_21
#define CC2650STK_BUZZER_ON             1
#define CC2650STK_BUZZER_OFF            0

/* DevPack */
#define CC2650STK_DP0                   IOID_25
#define CC2650STK_DP1                   IOID_24
#define CC2650STK_DP2                   IOID_23
#define CC2650STK_DP3                   IOID_27
#define CC2650STK_DP4_UARTRX            IOID_28
#define CC2650STK_DP5_UARTTX            IOID_29
#define CC2650STK_DP6_ADO               IOID_22
#define CC2650STK_DP7_BCLK              IOID_3
#define CC2650STK_DP8_TDI               IOID_17
#define CC2650STK_DP9_MISO              IOID_18
#define CC2650STK_DP10_MOSI             IOID_19
#define CC2650STK_DP11_CSN              IOID_20
#define CC2650STK_DP12_WCLK             IOID_16
#define CC2650STK_DP_ID                 IOID_30

/* Discrete Inputs */
#define CC2650STK_PIN_BTN1              IOID_4
#define CC2650STK_PIN_BTN2              IOID_0
#define CC2650STK_KEY_LEFT              CC2650STK_PIN_BTN2
#define CC2650STK_KEY_RIGHT             CC2650STK_PIN_BTN1
#define CC2650STK_RELAY                 IOID_3

/* GPIO */
#define CC2650STK_GPIO_LED_ON           1
#define CC2650STK_GPIO_LED_OFF          0

/* I2C */
#define CC2650STK_I2C0_SCL0             IOID_6
#define CC2650STK_I2C0_SDA0             IOID_5
#define CC2650STK_I2C0_SCL1             IOID_9
#define CC2650STK_I2C0_SDA1             IOID_8

/* I2S */
#define CC2650STK_I2S_ADO               IOID_22
#define CC2650STK_I2S_ADI               IOID_2
#define CC2650STK_I2S_BCLK              IOID_3
#define CC2650STK_I2S_MCLK              PIN_UNASSIGNED
#define CC2650STK_I2S_WCLK              IOID_16

/* LEDs */
#define CC2650STK_PIN_LED_ON            1
#define CC2650STK_PIN_LED_OFF           0
#define CC2650STK_PIN_RLED              IOID_10
#define CC2650STK_PIN_GLED              IOID_15

/* LED-Audio DevPack */
#define CC2650STK_DEVPK_LIGHT_BLUE      IOID_23
#define CC2650STK_DEVPK_LIGHT_GREEN     IOID_24
#define CC2650STK_DEVPK_LIGHT_WHITE     IOID_25
#define CC2650STK_DEVPK_LIGHT_RED       IOID_27

/* Power */
#define CC2650STK_MPU_POWER             IOID_12
#define CC2650STK_MPU_POWER_ON          1
#define CC2650STK_MPU_POWER_OFF         0

/* PWM */
#define CC2650STK_PWMPIN0               CC2650STK_PIN_RLED
#define CC2650STK_PWMPIN1               CC2650STK_PIN_GLED
#define CC2650STK_PWMPIN2               PIN_UNASSIGNED
#define CC2650STK_PWMPIN3               PIN_UNASSIGNED
#define CC2650STK_PWMPIN4               PIN_UNASSIGNED
#define CC2650STK_PWMPIN5               PIN_UNASSIGNED
#define CC2650STK_PWMPIN6               PIN_UNASSIGNED
#define CC2650STK_PWMPIN7               PIN_UNASSIGNED

/* Sensors */
#define CC2650STK_MPU_INT               IOID_7
#define CC2650STK_TMP_RDY               IOID_1

/* SPI */
#define CC2650STK_SPI_FLASH_CS          IOID_14
#define CC2650STK_FLASH_CS_ON           0
#define CC2650STK_FLASH_CS_OFF          1

/* SPI Board */
#define CC2650STK_SPI0_MISO             IOID_18
#define CC2650STK_SPI0_MOSI             IOID_19
#define CC2650STK_SPI0_CLK              IOID_17
#define CC2650STK_SPI0_CSN              IOID_20
#define CC2650STK_SPI1_MISO             PIN_UNASSIGNED
#define CC2650STK_SPI1_MOSI             PIN_UNASSIGNED
#define CC2650STK_SPI1_CLK              PIN_UNASSIGNED
#define CC2650STK_SPI1_CSN              PIN_UNASSIGNED

/* UART */
#define CC2650STK_UART_TX               CC2650STK_DP5_UARTTX
#define CC2650STK_UART_RX               CC2650STK_DP4_UARTRX

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
void CC2650STK_initGeneral(void);

/*!
 *  @brief  Turn off the external flash on LaunchPads
 *
 */
void CC2650STK_shutDownExtFlash(void);

/*!
 *  @brief  Wake up the external flash present on the board files
 *
 *  This function toggles the chip select for the amount of time needed
 *  to wake the chip up.
 */
void CC2650STK_wakeUpExtFlash(void);

/*!
 *  @def    CC2650STK_ADCBufName
 *  @brief  Enum of ADCBufs
 */
typedef enum CC2650STK_ADCBufName {
    CC2650STK_ADCBUF0 = 0,

    CC2650STK_ADCBUFCOUNT
} CC2650STK_ADCBufName;

/*!
 *  @def    CC2650STK_ADCBuf0ChannelName
 *  @brief  Enum of ADCBuf channels
 */
typedef enum CC2650STK_ADCBuf0ChannelName {
    CC2650STK_ADCBUF0CHANNEL0 = 0,
    CC2650STK_ADCBUF0CHANNEL1,
    CC2650STK_ADCBUF0CHANNEL2,
    CC2650STK_ADCBUF0CHANNEL3,
    CC2650STK_ADCBUF0CHANNEL4,
    CC2650STK_ADCBUF0CHANNEL5,
    CC2650STK_ADCBUF0CHANNEL6,
    CC2650STK_ADCBUF0CHANNEL7,
    CC2650STK_ADCBUF0CHANNELVDDS,
    CC2650STK_ADCBUF0CHANNELDCOUPL,
    CC2650STK_ADCBUF0CHANNELVSS,

    CC2650STK_ADCBUF0CHANNELCOUNT
} CC2650STK_ADCBuf0ChannelName;

/*!
 *  @def    CC2650STK_ADCName
 *  @brief  Enum of ADCs
 */
typedef enum CC2650STK_ADCName {
    CC2650STK_ADC0 = 0,
    CC2650STK_ADC1,
    CC2650STK_ADC2,
    CC2650STK_ADC3,
    CC2650STK_ADC4,
    CC2650STK_ADC5,
    CC2650STK_ADC6,
    CC2650STK_ADC7,
    CC2650STK_ADCDCOUPL,
    CC2650STK_ADCVSS,
    CC2650STK_ADCVDDS,

    CC2650STK_ADCCOUNT
} CC2650STK_ADCName;

/*!
 *  @def    CC2650STK_CryptoName
 *  @brief  Enum of Crypto names
 */
typedef enum CC2650STK_CryptoName {
    CC2650STK_CRYPTO0 = 0,

    CC2650STK_CRYPTOCOUNT
} CC2650STK_CryptoName;

/*!
 *  @def    CC2650STK_AESCCMName
 *  @brief  Enum of AESCCM names
 */
typedef enum CC2650STK_AESCCMName {
    CC2650STK_AESCCM0 = 0,

    CC2650STK_AESCCMCOUNT
} CC2650STK_AESCCMName;

/*!
 *  @def    CC2650STK_AESGCMName
 *  @brief  Enum of AESGCM names
 */
typedef enum CC2650STK_AESGCMName {
    CC2650STK_AESGCM0 = 0,

    CC2650STK_AESGCMCOUNT
} CC2650STK_AESGCMName;

/*!
 *  @def    CC2650STK_AESCBCName
 *  @brief  Enum of AESCBC names
 */
typedef enum CC2650STK_AESCBCName {
    CC2650STK_AESCBC0 = 0,

    CC2650STK_AESCBCCOUNT
} CC2650STK_AESCBCName;

/*!
 *  @def    CC2650STK_AESCTRName
 *  @brief  Enum of AESCTR names
 */
typedef enum CC2650STK_AESCTRName {
    CC2650STK_AESCTR0 = 0,

    CC2650STK_AESCTRCOUNT
} CC2650STK_AESCTRName;

/*!
 *  @def    CC2650STK_AESECBName
 *  @brief  Enum of AESECB names
 */
typedef enum CC2650STK_AESECBName {
    CC2650STK_AESECB0 = 0,

    CC2650STK_AESECBCOUNT
} CC2650STK_AESECBName;

/*!
 *  @def    CC2650STK_AESCTRDRBGName
 *  @brief  Enum of AESCTRDRBG names
 */
typedef enum CC2650STK_AESCTRDRBGName {
    CC2650STK_AESCTRDRBG0 = 0,

    CC2650STK_AESCTRDRBGCOUNT
} CC2650STK_AESCTRDRBGName;

/*!
 *  @def    CC2650STK_TRNGName
 *  @brief  Enum of TRNG names
 */
typedef enum CC2650STK_TRNGName {
    CC2650STK_TRNG0 = 0,

    CC2650STK_TRNGCOUNT
} CC2650STK_TRNGName;

/*!
 *  @def    CC2650STK_GPIOName
 *  @brief  Enum of GPIO names
 */
typedef enum CC2650STK_GPIOName {
    CC2650STK_GPIO_S1 = 0,
    CC2650STK_GPIO_S2,
    CC2650STK_GPIO_LED0,
    CC2650STK_GPIO_SPI_FLASH_CS,

    CC2650STK_GPIOCOUNT
} CC2650STK_GPIOName;

/*!
 *  @def    CC2650STK_GPTimerName
 *  @brief  Enum of GPTimer parts
 */
typedef enum CC2650STK_GPTimerName {
    CC2650STK_GPTIMER0A = 0,
    CC2650STK_GPTIMER0B,
    CC2650STK_GPTIMER1A,
    CC2650STK_GPTIMER1B,
    CC2650STK_GPTIMER2A,
    CC2650STK_GPTIMER2B,
    CC2650STK_GPTIMER3A,
    CC2650STK_GPTIMER3B,

    CC2650STK_GPTIMERPARTSCOUNT
} CC2650STK_GPTimerName;

/*!
 *  @def    CC2650STK_GPTimers
 *  @brief  Enum of GPTimers
 */
typedef enum CC2650STK_GPTimers {
    CC2650STK_GPTIMER0 = 0,
    CC2650STK_GPTIMER1,
    CC2650STK_GPTIMER2,
    CC2650STK_GPTIMER3,

    CC2650STK_GPTIMERCOUNT
} CC2650STK_GPTimers;

/*!
 *  @def    CC2650STK_I2CName
 *  @brief  Enum of I2C names
 */
typedef enum CC2650STK_I2CName {
#if TI_I2C_CONF_I2C0_ENABLE
    CC2650STK_I2C0 = 0,
    CC2650STK_I2C1 = 1,
#endif

    CC2650STK_I2CCOUNT
} CC2650STK_I2CName;

/*!
 *  @def    CC2650STK_I2SName
 *  @brief  Enum of I2S names
 */
typedef enum CC2650STK_I2SName {
    CC2650STK_I2S0 = 0,

    CC2650STK_I2SCOUNT
} CC2650STK_I2SName;

/*!
 *  @def    CC2650STK_NVSName
 *  @brief  Enum of NVS names
 */
typedef enum CC2650STK_NVSName {
#if TI_NVS_CONF_NVS_INTERNAL_ENABLE
    CC2650STK_NVSCC26XX0 = 0,
#endif
#if TI_NVS_CONF_NVS_EXTERNAL_ENABLE
    CC2650STK_NVSSPI25X0,
#endif

    CC2650STK_NVSCOUNT
} CC2650STK_NVSName;

/*!
 *  @def    CC2650STK_PWMName
 *  @brief  Enum of PWM outputs
 */
typedef enum CC2650STK_PWMName {
    CC2650STK_PWM0 = 0,
    CC2650STK_PWM1,
    CC2650STK_PWM2,
    CC2650STK_PWM3,
    CC2650STK_PWM4,
    CC2650STK_PWM5,
    CC2650STK_PWM6,
    CC2650STK_PWM7,

    CC2650STK_PWMCOUNT
} CC2650STK_PWMName;

/*!
 *  @def    CC2650STK_SPIName
 *  @brief  Enum of SPI names
 */
typedef enum CC2650STK_SPIName {
#if TI_SPI_CONF_SPI0_ENABLE
    CC2650STK_SPI0 = 0,
#endif
#if TI_SPI_CONF_SPI1_ENABLE
    CC2650STK_SPI1,
#endif

    CC2650STK_SPICOUNT
} CC2650STK_SPIName;

/*!
 *  @def    CC2650STK_UARTName
 *  @brief  Enum of UARTs
 */
typedef enum CC2650STK_UARTName {
#if TI_UART_CONF_UART0_ENABLE
    CC2650STK_UART0 = 0,
#endif


    CC2650STK_UARTCOUNT
} CC2650STK_UARTName;

/*!
 *  @def    CC2650STK_UDMAName
 *  @brief  Enum of DMA buffers
 */
typedef enum CC2650STK_UDMAName {
    CC2650STK_UDMA0 = 0,

    CC2650STK_UDMACOUNT
} CC2650STK_UDMAName;

/*!
 *  @def    CC2650STK_WatchdogName
 *  @brief  Enum of Watchdogs
 */
typedef enum CC2650STK_WatchdogName {
    CC2650STK_WATCHDOG0 = 0,

    CC2650STK_WATCHDOGCOUNT
} CC2650STK_WatchdogName;

#ifdef __cplusplus
}
#endif

#endif /* __CC2650STK_BOARD_H__ */

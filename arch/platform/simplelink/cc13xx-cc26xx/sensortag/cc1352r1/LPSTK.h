/*
 * Copyright (c) 2017-2019, Texas Instruments Incorporated
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
/** ===========================================================================
 *  @file       LPSTK.h
 *
 *  @brief      LPSTK Board Specific header file.
 *
 *  The LPSTK header file should be included in an application as
 *  follows:
 *  @code
 *  #include "LPSTK.h"
 *  @endcode
 *
 *  ===========================================================================
 */
#ifndef __LPSTK_BOARD_H__
#define __LPSTK_BOARD_H__

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
#define LPSTK

/* Mapping of pins to board signals using general board aliases
 *      <board signal alias>         <pin mapping>   <comments>
 */

/* Mapping of pins to board signals using general board aliases
 *      <board signal alias>                  <pin mapping>
 */
/* Analog Capable DIOs */
#define LPSTK_DIO24_ANALOG          IOID_24
#define LPSTK_DIO26_ANALOG          IOID_26
#define LPSTK_DIO28_ANALOG          IOID_28
#define LPSTK_DIO29_ANALOG          IOID_29

/* Buzzer */
#define LPSTK_BUZZER                IOID_21
#define LPSTK_BUZZER_ON             1
#define LPSTK_BUZZER_OFF            0

/* Discrete Inputs */
#define LPSTK_PIN_BTN1              IOID_15
#define LPSTK_PIN_BTN2              IOID_14
#define LPSTK_KEY_LEFT              LPSTK_PIN_BTN2
#define LPSTK_KEY_RIGHT             LPSTK_PIN_BTN1
#define LPSTK_RELAY                 IOID_23

/* GPIO */
#define LPSTK_GPIO_LED_ON           1
#define LPSTK_GPIO_LED_OFF          0

/* I2C */
#define LPSTK_I2C0_SCL0             IOID_4
#define LPSTK_I2C0_SDA0             IOID_5
#define LPSTK_I2C0_SCL1             PIN_UNASSIGNED
#define LPSTK_I2C0_SDA1             PIN_UNASSIGNED

/* I2S */
#define LPSTK_I2S_ADO               IOID_22
#define LPSTK_I2S_ADI               IOID_2
#define LPSTK_I2S_BCLK              IOID_3
#define LPSTK_I2S_MCLK              PIN_UNASSIGNED
#define LPSTK_I2S_WCLK              IOID_16

/* LEDs */
#define LPSTK_PIN_LED_ON            1
#define LPSTK_PIN_LED_OFF           0
#define LPSTK_PIN_RLED              IOID_6
#define LPSTK_PIN_GLED              IOID_7

/* Power */
#define LPSTK_MPU_POWER             IOID_11
#define LPSTK_MPU_POWER_ON          1
#define LPSTK_MPU_POWER_OFF         0

/* PWM Outputs */
#define LPSTK_PWMPIN0               LPSTK_PIN_RLED
#define LPSTK_PWMPIN1               LPSTK_PIN_GLED
#define LPSTK_PWMPIN2               PIN_UNASSIGNED
#define LPSTK_PWMPIN3               PIN_UNASSIGNED
#define LPSTK_PWMPIN4               PIN_UNASSIGNED
#define LPSTK_PWMPIN5               PIN_UNASSIGNED
#define LPSTK_PWMPIN6               PIN_UNASSIGNED
#define LPSTK_PWMPIN7               PIN_UNASSIGNED

/* Sensors */
#define LPSTK_MPU_INT               IOID_30
#define LPSTK_TMP_RDY               IOID_25

/* SPI */
#define LPSTK_SPI_FLASH_CS          IOID_20
#define LPSTK_FLASH_CS_ON           0
#define LPSTK_FLASH_CS_OFF          1

/* SPI Board */
#define LPSTK_SPI0_MISO             IOID_8          /* RF1.20 */
#define LPSTK_SPI0_MOSI             IOID_9          /* RF1.18 */
#define LPSTK_SPI0_CLK              IOID_10         /* RF1.16 */
#define LPSTK_SPI0_CSN              IOID_11
#define LPSTK_SPI1_MISO             PIN_UNASSIGNED
#define LPSTK_SPI1_MOSI             PIN_UNASSIGNED
#define LPSTK_SPI1_CLK              PIN_UNASSIGNED
#define LPSTK_SPI1_CSN              PIN_UNASSIGNED

/* UART Board */
#define LPSTK_UART0_RX              IOID_12         /* RXD */
#define LPSTK_UART0_TX              IOID_13         /* TXD */
#define LPSTK_UART0_CTS             IOID_19         /* CTS */
#define LPSTK_UART0_RTS             IOID_18         /* RTS */
#define LPSTK_UART1_RX              PIN_UNASSIGNED
#define LPSTK_UART1_TX              PIN_UNASSIGNED
#define LPSTK_UART1_CTS             PIN_UNASSIGNED
#define LPSTK_UART1_RTS             PIN_UNASSIGNED
/* For backward compatibility */
#define LPSTK_UART_RX               LPSTK_UART0_RX
#define LPSTK_UART_TX               LPSTK_UART0_TX
#define LPSTK_UART_CTS              LPSTK_UART0_CTS
#define LPSTK_UART_RTS              LPSTK_UART0_RTS

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
void LPSTK_initGeneral(void);

/*!
 *  @brief  Shut down the external flash present on the board files
 *
 *  This function bitbangs the SPI sequence necessary to turn off
 *  the external flash on LaunchPads.
 */
void LPSTK_shutDownExtFlash(void);

/*!
 *  @brief  Wake up the external flash present on the board files
 *
 *  This function toggles the chip select for the amount of time needed
 *  to wake the chip up.
 */
void LPSTK_wakeUpExtFlash(void);

/*!
 *  @def    LPSTK_ADCBufName
 *  @brief  Enum of ADCs
 */
typedef enum LPSTK_ADCBufName {
    LPSTK_ADCBUF0 = 0,

    LPSTK_ADCBUFCOUNT
} LPSTK_ADCBufName;

/*!
 *  @def    LPSTK_ADCBuf0ChannelName
 *  @brief  Enum of ADCBuf channels
 */
typedef enum LPSTK_ADCBuf0ChannelName {
    LPSTK_ADCBUF0CHANNEL0 = 0,
    LPSTK_ADCBUF0CHANNEL1,
    LPSTK_ADCBUF0CHANNEL2,
    LPSTK_ADCBUF0CHANNEL3,
    LPSTK_ADCBUF0CHANNEL4,
    LPSTK_ADCBUF0CHANNEL5,
    LPSTK_ADCBUF0CHANNEL6,
    LPSTK_ADCBUF0CHANNELVDDS,
    LPSTK_ADCBUF0CHANNELDCOUPL,
    LPSTK_ADCBUF0CHANNELVSS,

    LPSTK_ADCBUF0CHANNELCOUNT
} LPSTK_ADCBuf0ChannelName;

/*!
 *  @def    LPSTK_ADCName
 *  @brief  Enum of ADCs
 */
typedef enum LPSTK_ADCName {
    LPSTK_ADC0 = 0,
    LPSTK_ADC1,
    LPSTK_ADC2,
    LPSTK_ADC3,
    LPSTK_ADC4,
    LPSTK_ADC5,
    LPSTK_ADC6,
    LPSTK_ADCDCOUPL,
    LPSTK_ADCVSS,
    LPSTK_ADCVDDS,

    LPSTK_ADCCOUNT
} LPSTK_ADCName;

/*!
 *  @def    LPSTK_ECDHName
 *  @brief  Enum of ECDH names
 */
typedef enum LPSTK_ECDHName {
    LPSTK_ECDH0 = 0,

    LPSTK_ECDHCOUNT
} LPSTK_ECDHName;

/*!
 *  @def    LPSTK_ECDSAName
 *  @brief  Enum of ECDSA names
 */
typedef enum LPSTK_ECDSAName {
    LPSTK_ECDSA0 = 0,

    LPSTK_ECDSACOUNT
} LPSTK_ECDSAName;

/*!
 *  @def    LPSTK_ECJPAKEName
 *  @brief  Enum of ECJPAKE names
 */
typedef enum LPSTK_ECJPAKEName {
    LPSTK_ECJPAKE0 = 0,

    LPSTK_ECJPAKECOUNT
} LPSTK_ECJPAKEName;

/*!
 *  @def    LPSTK_AESCCMName
 *  @brief  Enum of AESCCM names
 */
typedef enum LPSTK_AESCCMName {
    LPSTK_AESCCM0 = 0,

    LPSTK_AESCCMCOUNT
} LPSTK_AESCCMName;

/*!
 *  @def    LPSTK_AESGCMName
 *  @brief  Enum of AESGCM names
 */
typedef enum LPSTK_AESGCMName {
    LPSTK_AESGCM0 = 0,

    LPSTK_AESGCMCOUNT
} LPSTK_AESGCMName;

/*!
 *  @def    LPSTK_AESCBCName
 *  @brief  Enum of AESCBC names
 */
typedef enum LPSTK_AESCBCName {
    LPSTK_AESCBC0 = 0,

    LPSTK_AESCBCCOUNT
} LPSTK_AESCBCName;

/*!
 *  @def    LPSTK_AESCTRName
 *  @brief  Enum of AESCTR names
 */
typedef enum LPSTK_AESCTRName {
    LPSTK_AESCTR0 = 0,

    LPSTK_AESCTRCOUNT
} LPSTK_AESCTRName;

/*!
 *  @def    LPSTK_AESECBName
 *  @brief  Enum of AESECB names
 */
typedef enum LPSTK_AESECBName {
    LPSTK_AESECB0 = 0,

    LPSTK_AESECBCOUNT
} LPSTK_AESECBName;

/*!
 *  @def    LPSTK_AESCTRDRBGName
 *  @brief  Enum of AESCTRDRBG names
 */
typedef enum LPSTK_AESCTRDRBGName {
    LPSTK_AESCTRDRBG0 = 0,

    LPSTK_AESCTRDRBGCOUNT
} LPSTK_AESCTRDRBGName;

/*!
 *  @def    LPSTK_SHA2Name
 *  @brief  Enum of SHA2 names
 */
typedef enum LPSTK_SHA2Name {
    LPSTK_SHA20 = 0,

    LPSTK_SHA2COUNT
} LPSTK_SHA2Name;

/*!
 *  @def    LPSTK_TRNGName
 *  @brief  Enum of TRNG names
 */
typedef enum LPSTK_TRNGName {
    LPSTK_TRNG0 = 0,

    LPSTK_TRNGCOUNT
} LPSTK_TRNGName;

/*!
 *  @def    LPSTK_GPIOName
 *  @brief  Enum of GPIO names
 */
typedef enum LPSTK_GPIOName {
    LPSTK_GPIO_S1 = 0,
    LPSTK_GPIO_S2,
    LPSTK_GPIO_LED_GREEN,
    LPSTK_GPIO_LED_RED,
    LPSTK_GPIO_SPI_FLASH_CS,

    LPSTK_GPIOCOUNT
} LPSTK_GPIOName;

/*!
 *  @def    LPSTK_GPTimerName
 *  @brief  Enum of GPTimer parts
 */
typedef enum LPSTK_GPTimerName {
    LPSTK_GPTIMER0A = 0,
    LPSTK_GPTIMER0B,
    LPSTK_GPTIMER1A,
    LPSTK_GPTIMER1B,
    LPSTK_GPTIMER2A,
    LPSTK_GPTIMER2B,
    LPSTK_GPTIMER3A,
    LPSTK_GPTIMER3B,

    LPSTK_GPTIMERPARTSCOUNT
} LPSTK_GPTimerName;

/*!
 *  @def    LPSTK_GPTimers
 *  @brief  Enum of GPTimers
 */
typedef enum LPSTK_GPTimers {
    LPSTK_GPTIMER0 = 0,
    LPSTK_GPTIMER1,
    LPSTK_GPTIMER2,
    LPSTK_GPTIMER3,

    LPSTK_GPTIMERCOUNT
} LPSTK_GPTimers;

/*!
 *  @def    LPSTK_I2CName
 *  @brief  Enum of I2C names
 */
typedef enum LPSTK_I2CName {
#if TI_I2C_CONF_I2C0_ENABLE
    LPSTK_I2C0 = 0,
    LPSTK_I2C1 = 1,
#endif

    LPSTK_I2CCOUNT
} LPSTK_I2CName;

/*!
 *  @def    LPSTK_I2SName
 *  @brief  Enum of I2S names
 */
typedef enum LPSTK_I2SName {
    LPSTK_I2S0 = 0,

    LPSTK_I2SCOUNT
} LPSTK_I2SName;

/*!
 *  @def    LPSTK_PDMName
 *  @brief  Enum of I2S names
 */
typedef enum LPSTK_PDMCOUNT {
    LPSTK_PDM0 = 0,

    LPSTK_PDMCOUNT
} LPSTK_PDMName;

/*!
 *  @def    LPSTK_NVSName
 *  @brief  Enum of NVS names
 */
typedef enum LPSTK_NVSName {
#if TI_NVS_CONF_NVS_INTERNAL_ENABLE
    LPSTK_NVSCC26XX0 = 0,
#endif
#if TI_NVS_CONF_NVS_EXTERNAL_ENABLE
    LPSTK_NVSSPI25X0,
#endif

    LPSTK_NVSCOUNT
} LPSTK_NVSName;

/*!
 *  @def    LPSTK_PWMName
 *  @brief  Enum of PWM outputs
 */
typedef enum LPSTK_PWMName {
    LPSTK_PWM0 = 0,
    LPSTK_PWM1,
    LPSTK_PWM2,
    LPSTK_PWM3,
    LPSTK_PWM4,
    LPSTK_PWM5,
    LPSTK_PWM6,
    LPSTK_PWM7,

    LPSTK_PWMCOUNT
} LPSTK_PWMName;

/*!
 *  @def    LPSTK_SPIName
 *  @brief  Enum of SPI names
 */
typedef enum LPSTK_SPIName {
#if TI_SPI_CONF_SPI0_ENABLE
    LPSTK_SPI0 = 0,
#endif
#if TI_SPI_CONF_SPI1_ENABLE
    LPSTK_SPI1,
#endif

    LPSTK_SPICOUNT
} LPSTK_SPIName;

/*!
 *  @def    LPSTK_UARTName
 *  @brief  Enum of UARTs
 */
typedef enum LPSTK_UARTName {
#if TI_UART_CONF_UART0_ENABLE
    LPSTK_UART0 = 0,
#endif

    LPSTK_UARTCOUNT
} LPSTK_UARTName;

/*!
 *  @def    LPSTK_UDMAName
 *  @brief  Enum of DMA buffers
 */
typedef enum LPSTK_UDMAName {
    LPSTK_UDMA0 = 0,

    LPSTK_UDMACOUNT
} LPSTK_UDMAName;

/*!
 *  @def    LPSTK_WatchdogName
 *  @brief  Enum of Watchdogs
 */
typedef enum LPSTK_WatchdogName {
    LPSTK_WATCHDOG0 = 0,

    LPSTK_WATCHDOGCOUNT
} LPSTK_WatchdogName;


#ifdef __cplusplus
}
#endif

#endif /* __LPSTK_BOARD_H__ */

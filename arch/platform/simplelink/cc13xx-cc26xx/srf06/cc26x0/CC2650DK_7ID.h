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
 *  @file       CC2650DK_7ID.h
 *
 *  @brief      CC2650 LaunchPad Board Specific header file.
 *
 *  The CC2650DK_7ID header file should be included in an application as
 *  follows:
 *  @code
 *  #include "CC2650DK_7ID.h"
 *  @endcode
 *
 *  ============================================================================
 */
#ifndef __CC2650DK_7ID_BOARD_H__
#define __CC2650DK_7ID_BOARD_H__

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
#define CC2650DK_7ID

/* Mapping of pins to board signals using general board aliases
 *      <board signal alias>        <pin mapping>   <comments>
 */

/* Analog Capable DIOs */
#define CC2650DK_7ID_DIO23_ANALOG          IOID_23
#define CC2650DK_7ID_DIO24_ANALOG          IOID_24
#define CC2650DK_7ID_DIO25_ANALOG          IOID_25
#define CC2650DK_7ID_DIO26_ANALOG          IOID_26
#define CC2650DK_7ID_DIO27_ANALOG          IOID_27
#define CC2650DK_7ID_DIO28_ANALOG          IOID_28
#define CC2650DK_7ID_DIO29_ANALOG          IOID_29
#define CC2650DK_7ID_DIO30_ANALOG          IOID_30

/* Digital IOs */
#define CC2650DK_7ID_DIO0                  IOID_0
#define CC2650DK_7ID_DIO1_RFSW             IOID_1
#define CC2650DK_7ID_DIO12                 IOID_12
#define CC2650DK_7ID_DIO15                 IOID_15
#define CC2650DK_7ID_DIO16_TDO             IOID_16
#define CC2650DK_7ID_DIO17_TDI             IOID_17
#define CC2650DK_7ID_DIO21                 IOID_21
#define CC2650DK_7ID_DIO22                 IOID_22

/* Discrete Inputs */
#define CC2650DK_7ID_PIN_KEY_SELECT        IOID_11
#define CC2650DK_7ID_PIN_KEY_UP            IOID_19
#define CC2650DK_7ID_PIN_KEY_DOWN          IOID_12
#define CC2650DK_7ID_PIN_KEY_LEFT          IOID_15
#define CC2650DK_7ID_PIN_KEY_RIGHT         IOID_18

/* GPIO */
#define CC2650DK_7ID_GPIO_LED_ON           1
#define CC2650DK_7ID_GPIO_LED_OFF          0

/* I2C */
#define CC2650DK_7ID_I2C0_SCL0             PIN_UNASSIGNED
#define CC2650DK_7ID_I2C0_SDA0             PIN_UNASSIGNED

/* I2S */
#define CC2650DK_7ID_I2S_ADO               PIN_UNASSIGNED
#define CC2650DK_7ID_I2S_ADI               PIN_UNASSIGNED
#define CC2650DK_7ID_I2S_BCLK              PIN_UNASSIGNED
#define CC2650DK_7ID_I2S_MCLK              PIN_UNASSIGNED
#define CC2650DK_7ID_I2S_WCLK              PIN_UNASSIGNED

/* LEDs */
#define CC2650DK_7ID_PIN_LED_ON            1
#define CC2650DK_7ID_PIN_LED_OFF           0
#define CC2650DK_7ID_PIN_LED1              IOID_25
#define CC2650DK_7ID_PIN_LED2              IOID_27
#define CC2650DK_7ID_PIN_LED3              IOID_7
#define CC2650DK_7ID_PIN_LED4              IOID_6

/* PWM Outputs */
#define CC2650DK_7ID_PWMPIN0               CC2650DK_7ID_PIN_LED1
#define CC2650DK_7ID_PWMPIN1               CC2650DK_7ID_PIN_LED2
#define CC2650DK_7ID_PWMPIN2               CC2650DK_7ID_PIN_LED3
#define CC2650DK_7ID_PWMPIN3               CC2650DK_7ID_PIN_LED4
#define CC2650DK_7ID_PWMPIN4               PIN_UNASSIGNED
#define CC2650DK_7ID_PWMPIN5               PIN_UNASSIGNED
#define CC2650DK_7ID_PWMPIN6               PIN_UNASSIGNED
#define CC2650DK_7ID_PWMPIN7               PIN_UNASSIGNED

/* SPI Board */
#define CC2650DK_7ID_SPI0_MISO             IOID_8
#define CC2650DK_7ID_SPI0_MOSI             IOID_9
#define CC2650DK_7ID_SPI0_CLK              IOID_10
#define CC2650DK_7ID_SPI0_CSN              PIN_UNASSIGNED
#define CC2650DK_7ID_SPI1_MISO             PIN_UNASSIGNED
#define CC2650DK_7ID_SPI1_MOSI             PIN_UNASSIGNED
#define CC2650DK_7ID_SPI1_CLK              PIN_UNASSIGNED
#define CC2650DK_7ID_SPI1_CSN              PIN_UNASSIGNED

/* UART Board */
#define CC2650DK_7ID_UART_RX               IOID_2
#define CC2650DK_7ID_UART_TX               IOID_3
#define CC2650DK_7ID_UART_CTS              IOID_0
#define CC2650DK_7ID_UART_RTS              IOID_21

/* SD Card */
#define CC2650DK_7ID_SPI_SDCARD_CS         IOID_30
#define CC2650DK_7ID_SDCARD_CS_ON          0
#define CC2650DK_7ID_SDCARD_CS_OFF         1

/* Ambient Light Sensor */
#define CC2650DK_7ID_ALS_OUT               IOID_23
#define CC2650DK_7ID_ALS_PWR               IOID_26

/* Accelerometer */
#define CC2650DK_7ID_ACC_PWR               IOID_20
#define CC2650DK_7ID_ACC_CS                IOID_24

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
void CC2650DK_7ID_initGeneral(void);

/*!
 *  @brief  Turn off the external flash on LaunchPads
 *
 */
void CC2650DK_7ID_shutDownExtFlash(void);

/*!
 *  @brief  Wake up the external flash present on the board files
 *
 *  This function toggles the chip select for the amount of time needed
 *  to wake the chip up.
 */
void CC2650DK_7ID_wakeUpExtFlash(void);

/*!
 *  @def    CC2650DK_7ID_ADCBufName
 *  @brief  Enum of ADCBufs
 */
typedef enum CC2650DK_7ID_ADCBufName {
    CC2650DK_7ID_ADCBUF0 = 0,

    CC2650DK_7ID_ADCBUFCOUNT
} CC2650DK_7ID_ADCBufName;

/*!
 *  @def    CC2650DK_7ID_ADCBuf0ChannelName
 *  @brief  Enum of ADCBuf channels
 */
typedef enum CC2650DK_7ID_ADCBuf0ChannelName {
    CC2650DK_7ID_ADCBUF0CHANNELADCALS = 0,
    CC2650DK_7ID_ADCBUF0CHANNELVDDS,
    CC2650DK_7ID_ADCBUF0CHANNELDCOUPL,
    CC2650DK_7ID_ADCBUF0CHANNELVSS,

    CC2650DK_7ID_ADCBUF0CHANNELCOUNT
} CC2650DK_7ID_ADCBuf0ChannelName;

/*!
 *  @def    CC2650DK_7ID_ADCName
 *  @brief  Enum of ADCs
 */
typedef enum CC2650DK_7ID_ADCName {
    CC2650DK_7ID_ADCALS = 0,
    CC2650DK_7ID_ADCDCOUPL,
    CC2650DK_7ID_ADCVSS,
    CC2650DK_7ID_ADCVDDS,

    CC2650DK_7ID_ADCCOUNT
} CC2650DK_7ID_ADCName;

/*!
 *  @def    CC2650DK_7ID_CryptoName
 *  @brief  Enum of Crypto names
 */
typedef enum CC2650DK_7ID_CryptoName {
    CC2650DK_7ID_CRYPTO0 = 0,

    CC2650DK_7ID_CRYPTOCOUNT
} CC2650DK_7ID_CryptoName;

/*!
 *  @def    CC2650DK_7ID_AESCCMName
 *  @brief  Enum of AESCCM names
 */
typedef enum CC2650DK_7ID_AESCCMName {
    CC2650DK_7ID_AESCCM0 = 0,

    CC2650DK_7ID_AESCCMCOUNT
} CC2650DK_7ID_AESCCMName;

/*!
 *  @def    CC2650DK_7ID_AESGCMName
 *  @brief  Enum of AESGCM names
 */
typedef enum CC2650DK_7ID_AESGCMName {
    CC2650DK_7ID_AESGCM0 = 0,

    CC2650DK_7ID_AESGCMCOUNT
} CC2650DK_7ID_AESGCMName;

/*!
 *  @def    CC2650DK_7ID_AESCBCName
 *  @brief  Enum of AESCBC names
 */
typedef enum CC2650DK_7ID_AESCBCName {
    CC2650DK_7ID_AESCBC0 = 0,

    CC2650DK_7ID_AESCBCCOUNT
} CC2650DK_7ID_AESCBCName;

/*!
 *  @def    CC2650DK_7ID_AESCTRName
 *  @brief  Enum of AESCTR names
 */
typedef enum CC2650DK_7ID_AESCTRName {
    CC2650DK_7ID_AESCTR0 = 0,

    CC2650DK_7ID_AESCTRCOUNT
} CC2650DK_7ID_AESCTRName;

/*!
 *  @def    CC2650DK_7ID_AESECBName
 *  @brief  Enum of AESECB names
 */
typedef enum CC2650DK_7ID_AESECBName {
    CC2650DK_7ID_AESECB0 = 0,

    CC2650DK_7ID_AESECBCOUNT
} CC2650DK_7ID_AESECBName;

/*!
 *  @def    CC2650DK_7ID_AESCTRDRBGName
 *  @brief  Enum of AESCTRDRBG names
 */
typedef enum CC2650DK_7ID_AESCTRDRBGName {
    CC2650DK_7ID_AESCTRDRBG0 = 0,

    CC2650DK_7ID_AESCTRDRBGCOUNT
} CC2650DK_7ID_AESCTRDRBGName;

/*!
 *  @def    CC2650DK_7ID_TRNGName
 *  @brief  Enum of TRNG names
 */
typedef enum CC2650DK_7ID_TRNGName {
    CC2650DK_7ID_TRNG0 = 0,

    CC2650DK_7ID_TRNGCOUNT
} CC2650DK_7ID_TRNGName;

/*!
 *  @def    CC2650DK_7ID_GPIOName
 *  @brief  Enum of GPIO names
 */
typedef enum CC2650DK_7ID_GPIOName {
    CC2650DK_7ID_GPIO_KEY_SELECT = 0,
    CC2650DK_7ID_GPIO_KEY_UP,
    CC2650DK_7ID_GPIO_KEY_DOWN,
    CC2650DK_7ID_GPIO_KEY_LEFT,
    CC2650DK_7ID_GPIO_KEY_RIGHT,
    CC2650DK_7ID_SPI_MASTER_READY,
    CC2650DK_7ID_SPI_SLAVE_READY,
    CC2650DK_7ID_GPIO_LED1,
    CC2650DK_7ID_GPIO_LED2,
    CC2650DK_7ID_GPIO_LED3,
    CC2650DK_7ID_GPIO_LED4,
    CC2650DK_7ID_SDSPI_CS,
    CC2650DK_7ID_GPIO_ACC_CS,

    CC2650DK_7ID_GPIOCOUNT
} CC2650DK_7ID_GPIOName;

/*!
 *  @def    CC2650DK_7ID_GPTimerName
 *  @brief  Enum of GPTimer parts
 */
typedef enum CC2650DK_7ID_GPTimerName {
    CC2650DK_7ID_GPTIMER0A = 0,
    CC2650DK_7ID_GPTIMER0B,
    CC2650DK_7ID_GPTIMER1A,
    CC2650DK_7ID_GPTIMER1B,
    CC2650DK_7ID_GPTIMER2A,
    CC2650DK_7ID_GPTIMER2B,
    CC2650DK_7ID_GPTIMER3A,
    CC2650DK_7ID_GPTIMER3B,

    CC2650DK_7ID_GPTIMERPARTSCOUNT
} CC2650DK_7ID_GPTimerName;

/*!
 *  @def    CC2650DK_7ID_GPTimers
 *  @brief  Enum of GPTimers
 */
typedef enum CC2650DK_7ID_GPTimers {
    CC2650DK_7ID_GPTIMER0 = 0,
    CC2650DK_7ID_GPTIMER1,
    CC2650DK_7ID_GPTIMER2,
    CC2650DK_7ID_GPTIMER3,

    CC2650DK_7ID_GPTIMERCOUNT
} CC2650DK_7ID_GPTimers;

/*!
 *  @def    CC2650DK_7ID_I2CName
 *  @brief  Enum of I2C names
 */
typedef enum CC2650DK_7ID_I2CName {
#if TI_I2C_CONF_I2C0_ENABLE
    CC2650DK_7ID_I2C0 = 0,
#endif

    CC2650DK_7ID_I2CCOUNT
} CC2650DK_7ID_I2CName;

/*!
 *  @def    CC2650DK_7ID_I2SName
 *  @brief  Enum of I2S names
 */
typedef enum CC2650DK_7ID_I2SName {
    CC2650DK_7ID_I2S0 = 0,

    CC2650DK_7ID_I2SCOUNT
} CC2650DK_7ID_I2SName;

/*!
 *  @def    CC2650DK_7ID_NVSName
 *  @brief  Enum of NVS names
 */
typedef enum CC2650DK_7ID_NVSName {
#if TI_NVS_CONF_NVS_INTERNAL_ENABLE
    CC2650DK_7ID_NVSCC26XX0 = 0,
#endif

    CC2650DK_7ID_NVSCOUNT
} CC2650DK_7ID_NVSName;

/*!
 *  @def    CC2650DK_7ID_PWMName
 *  @brief  Enum of PWM outputs
 */
typedef enum CC2650DK_7ID_PWMName {
    CC2650DK_7ID_PWM0 = 0,
    CC2650DK_7ID_PWM1,
    CC2650DK_7ID_PWM2,
    CC2650DK_7ID_PWM3,
    CC2650DK_7ID_PWM4,
    CC2650DK_7ID_PWM5,
    CC2650DK_7ID_PWM6,
    CC2650DK_7ID_PWM7,

    CC2650DK_7ID_PWMCOUNT
} CC2650DK_7ID_PWMName;

/*!
 *  @def    CC2650DK_7ID_SDName
 *  @brief  Enum of SD names
 */
typedef enum CC2650DK_7ID_SDName {
    CC2650DK_7ID_SDSPI0 = 0,

    CC2650DK_7ID_SDCOUNT
} CC2650DK_7ID_SDName;

/*!
 *  @def    CC2650DK_7ID_SPIName
 *  @brief  Enum of SPI names
 */
typedef enum CC2650DK_7ID_SPIName {
#if TI_SPI_CONF_SPI0_ENABLE
    CC2650DK_7ID_SPI0 = 0,
#endif
#if TI_SPI_CONF_SPI1_ENABLE
    CC2650DK_7ID_SPI1,
#endif

    CC2650DK_7ID_SPICOUNT
} CC2650DK_7ID_SPIName;

/*!
 *  @def    CC2650DK_7ID_UARTName
 *  @brief  Enum of UARTs
 */
typedef enum CC2650DK_7ID_UARTName {
#if TI_UART_CONF_UART0_ENABLE
    CC2650DK_7ID_UART0 = 0,
#endif

    CC2650DK_7ID_UARTCOUNT
} CC2650DK_7ID_UARTName;

/*!
 *  @def    CC2650DK_7ID_UDMAName
 *  @brief  Enum of DMA buffers
 */
typedef enum CC2650DK_7ID_UDMAName {
    CC2650DK_7ID_UDMA0 = 0,

    CC2650DK_7ID_UDMACOUNT
} CC2650DK_7ID_UDMAName;

/*!
 *  @def    CC2650DK_7ID_WatchdogName
 *  @brief  Enum of Watchdogs
 */
typedef enum CC2650DK_7ID_WatchdogName {
    CC2650DK_7ID_WATCHDOG0 = 0,

    CC2650DK_7ID_WATCHDOGCOUNT
} CC2650DK_7ID_WatchdogName;

#ifdef __cplusplus
}
#endif

#endif /* __CC2650DK_7ID_BOARD_H__ */

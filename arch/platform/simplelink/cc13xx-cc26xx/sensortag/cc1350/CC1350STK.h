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
 *  @file       CC1350STK.h
 *
 *  @brief      CC1350STK Board Specific header file.
 *
 *  The CC1350STK header file should be included in an application as
 *  follows:
 *  @code
 *  #include "CC1350STK.h"
 *  @endcode
 *  ============================================================================
 */
#ifndef __CC1350STK_BOARD_H__
#define __CC1350STK_BOARD_H__

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
#define CC1350STK

/* Mapping of pins to board signals using general board aliases
 *      <board signal alias>            <pin mapping>
 */

/* Audio */
#define CC1350STK_MIC_POWER             IOID_13
#define CC1350STK_MIC_POWER_ON          1
#define CC1350STK_MIC_POWER_OFF         0
#define CC1350STK_AUDIO_DI              IOID_2
#define CC1350STK_AUDIO_CLK             IOID_3

/* Buzzer */
#define CC1350STK_BUZZER                IOID_21
#define CC1350STK_BUZZER_ON             1
#define CC1350STK_BUZZER_OFF            0

/* DevPack */
#define CC1350STK_AUDIOFS_TDO           IOID_16
#define CC1350STK_AUDIODO               IOID_22
#define CC1350STK_DP2                   IOID_23
#define CC1350STK_DP1                   IOID_24
#define CC1350STK_DP0                   IOID_25
#define CC1350STK_DP3                   IOID_27
#define CC1350STK_DP4_UARTRX            IOID_28
#define CC1350STK_DP5_UARTTX            IOID_29
#define CC1350STK_DEVPK_ID              IOID_30
#define CC1350STK_SPI_DEVPK_CS          IOID_20

/* Discrete Outputs */
#define CC1350STK_PIN_LED1              IOID_10
#define CC1350STK_LED_ON                1
#define CC1350STK_LED_OFF               0


/* Discrete Inputs */
#define CC1350STK_KEY_LEFT              IOID_15
#define CC1350STK_KEY_RIGHT             IOID_4
#define CC1350STK_RELAY                 IOID_1

/* GPIO */
#define CC1350STK_GPIO_LED_ON           1
#define CC1350STK_GPIO_LED_OFF          0

/* I2C */
#define CC1350STK_I2C0_SDA0             IOID_5
#define CC1350STK_I2C0_SCL0             IOID_6
#define CC1350STK_I2C0_SDA1             IOID_8
#define CC1350STK_I2C0_SCL1             IOID_9

/* LED-Audio DevPack */
#define CC1350STK_DEVPK_LIGHT_BLUE      IOID_23
#define CC1350STK_DEVPK_LIGHT_GREEN     IOID_24
#define CC1350STK_DEVPK_LIGHT_WHITE     IOID_25
#define CC1350STK_DEVPK_LIGHT_RED       IOID_27

/* Power */
#define CC1350STK_MPU_POWER             IOID_12
#define CC1350STK_MPU_POWER_ON          1
#define CC1350STK_MPU_POWER_OFF         0

/* PWM */
#define CC1350STK_PWMPIN0               CC1350STK_PIN_LED1
#define CC1350STK_PWMPIN1               CC1350STK_PIN_LED1
#define CC1350STK_PWMPIN2               PIN_UNASSIGNED
#define CC1350STK_PWMPIN3               PIN_UNASSIGNED
#define CC1350STK_PWMPIN4               PIN_UNASSIGNED
#define CC1350STK_PWMPIN5               PIN_UNASSIGNED
#define CC1350STK_PWMPIN6               PIN_UNASSIGNED
#define CC1350STK_PWMPIN7               PIN_UNASSIGNED

/* Sensors */
#define CC1350STK_MPU_INT               IOID_7
#define CC1350STK_TMP_RDY               IOID_11

/* SPI */
#define CC1350STK_SPI_FLASH_CS          IOID_14
#define CC1350STK_FLASH_CS_ON           0
#define CC1350STK_FLASH_CS_OFF          1

/* SPI Board */
#define CC1350STK_SPI0_MISO             IOID_18
#define CC1350STK_SPI0_MOSI             IOID_19
#define CC1350STK_SPI0_CLK              IOID_17
#define CC1350STK_SPI0_CSN              PIN_UNASSIGNED
#define CC1350STK_SPI1_MISO             PIN_UNASSIGNED
#define CC1350STK_SPI1_MOSI             PIN_UNASSIGNED
#define CC1350STK_SPI1_CLK              PIN_UNASSIGNED
#define CC1350STK_SPI1_CSN              PIN_UNASSIGNED

/* UART */
#define CC1350STK_UART_TX               CC1350STK_DP5_UARTTX
#define CC1350STK_UART_RX               CC1350STK_DP4_UARTRX

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
void CC1350STK_initGeneral(void);

/*!
 *  @brief  Turn off the external flash on LaunchPads
 *
 */
void CC1350STK_shutDownExtFlash(void);

/*!
 *  @brief  Wake up the external flash present on the board files
 *
 *  This function toggles the chip select for the amount of time needed
 *  to wake the chip up.
 */
void CC1350STK_wakeUpExtFlash(void);

/*!
 *  @def    CC1350STK_CryptoName
 *  @brief  Enum of Crypto names
 */
typedef enum CC1350STK_CryptoName {
    CC1350STK_CRYPTO0 = 0,

    CC1350STK_CRYPTOCOUNT
} CC1350STK_CryptoName;

/*!
 *  @def    CC1350STK_GPIOName
 *  @brief  Enum of GPIO names
 */
typedef enum CC1350STK_GPIOName {
    CC1350STK_GPIO_S1 = 0,
    CC1350STK_GPIO_S2,
    CC1350STK_GPIO_LED0,
    CC1350STK_GPIO_SPI_FLASH_CS,

    CC1350STK_GPIOCOUNT
} CC1350STK_GPIOName;

/*!
 *  @def    CC1350STK_GPTimerName
 *  @brief  Enum of GPTimers parts
 */
typedef enum CC1350STK_GPTimerName {
    CC1350STK_GPTIMER0A = 0,
    CC1350STK_GPTIMER0B,
    CC1350STK_GPTIMER1A,
    CC1350STK_GPTIMER1B,
    CC1350STK_GPTIMER2A,
    CC1350STK_GPTIMER2B,
    CC1350STK_GPTIMER3A,
    CC1350STK_GPTIMER3B,

    CC1350STK_GPTIMERPARTSCOUNT
} CC1350STK_GPTimerName;

/*!
 *  @def    CC1350STK_GPTimers
 *  @brief  Enum of GPTimers
 */
typedef enum CC1350STK_GPTimers {
    CC1350STK_GPTIMER0 = 0,
    CC1350STK_GPTIMER1,
    CC1350STK_GPTIMER2,
    CC1350STK_GPTIMER3,

    CC1350STK_GPTIMERCOUNT
} CC1350STK_GPTimers;

/*!
 *  @def    CC1350STK_I2CName
 *  @brief  Enum of I2C names
 */
typedef enum CC1350STK_I2CName {
#if TI_I2C_CONF_I2C0_ENABLE
    CC1350STK_I2C0 = 0,
#endif

    CC1350STK_I2CCOUNT
} CC1350STK_I2CName;

/*!
 *  @def    CC1350STK_NVSName
 *  @brief  Enum of NVS names
 */
typedef enum CC1350STK_NVSName {
#if TI_NVS_CONF_NVS_INTERNAL_ENABLE
    CC1350STK_NVSCC26XX0 = 0,
#endif
#if TI_NVS_CONF_NVS_EXTERNAL_ENABLE
    CC1350STK_NVSSPI25X0,
#endif

    CC1350STK_NVSCOUNT
} CC1350STK_NVSName;

/*!
 *  @def    CC1350STK_PDMName
 *  @brief  Enum of PDM names
 */
typedef enum CC1350STK_PDMName {
    CC1350STK_PDM0 = 0,

    CC1350STK_PDMCOUNT
} CC1350STK_PDMName;

/*!
 *  @def    CC1350STK_PWMName
 *  @brief  Enum of PWM outputs
 */
typedef enum CC1350STK_PWMName {
    CC1350STK_PWM0 = 0,
    CC1350STK_PWM1,
    CC1350STK_PWM2,
    CC1350STK_PWM3,
    CC1350STK_PWM4,
    CC1350STK_PWM5,
    CC1350STK_PWM6,
    CC1350STK_PWM7,

    CC1350STK_PWMCOUNT
} CC1350STK_PWMName;

/*!
 *  @def    CC1350STK_SPIName
 *  @brief  Enum of SPI names
 */
typedef enum CC1350STK_SPIName {
#if TI_SPI_CONF_SPI0_ENABLE
    CC1350STK_SPI0 = 0,
#endif
#if TI_SPI_CONF_SPI1_ENABLE
    CC1350STK_SPI1,
#endif

    CC1350STK_SPICOUNT
} CC1350STK_SPIName;

/*!
 *  @def    CC1350STK_TRNGName
 *  @brief  Enum of TRNGs
 */
typedef enum CC1350STK_TRNGName {
    CC1350STK_TRNG0 = 0,

    CC1350STK_TRNGCOUNT
} CC1350STK_TRNGName;

/*!
 *  @def    CC1350STK_UARTName
 *  @brief  Enum of UARTs
 */
typedef enum CC1350STK_UARTName {
#if TI_UART_CONF_UART0_ENABLE
    CC1350STK_UART0 = 0,
#endif

    CC1350STK_UARTCOUNT
} CC1350STK_UARTName;

/*!
 *  @def    CC1350STK_UDMAName
 *  @brief  Enum of DMA buffers
 */
typedef enum CC1350STK_UDMAName {
    CC1350STK_UDMA0 = 0,

    CC1350STK_UDMACOUNT
} CC1350STK_UDMAName;

/*!
 *  @def    CC1350STK_WatchdogName
 *  @brief  Enum of Watchdogs
 */
typedef enum CC1350STK_WatchdogName {
    CC1350STK_WATCHDOG0 = 0,

    CC1350STK_WATCHDOGCOUNT
} CC1350STK_WatchdogName;

#ifdef __cplusplus
}
#endif

#endif /* __CC1350STK_BOARD_H__ */

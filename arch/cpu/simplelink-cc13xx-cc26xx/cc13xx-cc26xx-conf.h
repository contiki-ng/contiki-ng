/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \addtogroup cc13xx-cc26xx-cpu
 * @{
 *
 * \file
 *        Header with configuration defines common to the CC13xx/CC26xx
 *        platform.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef CC13XX_CC26XX_CONF_H_
#define CC13XX_CC26XX_CONF_H_
/*---------------------------------------------------------------------------*/
#include "cc13xx-cc26xx-def.h"

#include "rf/rf.h"
/*---------------------------------------------------------------------------*/
/**
 * \name Board Configuration.
 *
 * @{
 */

/* Configure that a board has sensors for the dev/sensors.h API. */
#ifndef BOARD_CONF_HAS_SENSORS
#define BOARD_CONF_HAS_SENSORS              0
#endif

/* Enable/disable the dev/sensors.h API for the given board. */
#ifndef BOARD_CONF_SENSORS_DISABLE
#define BOARD_CONF_SENSORS_DISABLE          0
#endif
/*---------------------------------------------------------------------------*/
/**
 * \name GPIO HAL configuration.
 *
 * @{
 */
#define GPIO_HAL_CONF_ARCH_SW_TOGGLE        0
#define GPIO_HAL_CONF_ARCH_HDR_PATH         "dev/gpio-hal-arch.h"
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Watchdog Configuration.
 *
 * @{
 */
#ifndef WATCHDOG_CONF_DISABLE
#define WATCHDOG_CONF_DISABLE               0
#endif

#ifndef WATCHDOG_CONF_TIMER_TOP
#define WATCHDOG_CONF_TIMER_TOP             0xFFFFF
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name RF configuration.
 *
 * @{
 */

/*
 * Set the inactivity timeout period for the RF driver. This determines how
 * long the RF driver will wait when inactive until turning off the RF Core.
 * Specified in microseconds.
 */
#ifndef RF_CONF_INACTIVITY_TIMEOUT
#define RF_CONF_INACTIVITY_TIMEOUT      2000    /**< 2 ms */
#endif

/*
 * Configure TX power to either default PA or High PA, defaults to
 * default PA.
 */
#ifndef RF_CONF_TXPOWER_HIGH_PA
#define RF_CONF_TXPOWER_HIGH_PA         0
#endif

#if (RF_CONF_TXPOWER_HIGH_PA) && !(SUPPORTS_HIGH_PA)
#error "Device does not support High PA"
#endif

/*
 * CC13xx only: Configure TX power to use boot mode, allowing to gain
 * up to 14 dBm with the default PA. This will, however, increase power
 * consumption.
 */
#ifndef RF_CONF_TXPOWER_BOOST_MODE
#define RF_CONF_TXPOWER_BOOST_MODE      0
#endif

/*
 * Configure RF mode. That is, whether to run on Sub-1 GHz (Prop-mode) or
 * 2.4 GHz (IEEE-mode).
 */
#ifdef RF_CONF_MODE
/* Sanity check a valid configuration is provided. */
#if !(RF_CONF_MODE & RF_MODE_BM)
#error "Invalid RF_CONF_MODE provided"
#endif

#define RF_MODE                      RF_CONF_MODE
#endif /* RF_CONF_MODE */

/* Number of RX buffers. */
#ifndef RF_CONF_RX_BUF_CNT
#define RF_CONF_RX_BUF_CNT           4
#endif

/* Size of each RX buffer in bytes. */
#ifndef RF_CONF_RX_BUF_SIZE
#define RF_CONF_RX_BUF_SIZE          144
#endif

/* Enable/disable BLE beacon. */
#ifndef RF_CONF_BLE_BEACON_ENABLE
#define RF_CONF_BLE_BEACON_ENABLE    0
#endif

#if (RF_CONF_BLE_BEACON_ENABLE) && !(SUPPORTS_BLE_BEACON)
#error "Device does not support BLE for BLE beacon"
#endif

/*----- CC13xx Device Line --------------------------------------------------*/
/* CC13xx supports both IEEE and Prop mode, depending on which device. */
#if defined(DEVICE_LINE_CC13XX)

/* Default to Prop-mode for CC13xx devices if not configured. */
#ifndef RF_MODE
#define RF_MODE                      RF_MODE_SUB_1_GHZ
#endif

/*----- CC13xx Prop-mode ----------------------------------------------------*/
#if (RF_MODE == RF_MODE_SUB_1_GHZ) && (SUPPORTS_PROP_MODE)

/* Netstack configuration. */
#define NETSTACK_CONF_RADIO         prop_mode_driver

/* CSMA configuration. */
#define CSMA_CONF_ACK_WAIT_TIME                (RTIMER_SECOND / 300)
#define CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME (RTIMER_SECOND / 1000)
#define CSMA_CONF_SEND_SOFT_ACK      1

/*----- CC13xx IEEE-mode ----------------------------------------------------*/
#elif (RF_MODE == RF_MODE_2_4_GHZ) && (SUPPORTS_IEEE_MODE)

/* Netstack configuration. */
#define NETSTACK_CONF_RADIO         ieee_mode_driver

/* CSMA configuration. */
#define CSMA_CONF_SEND_SOFT_ACK     0

#else
/*----- CC13xx Unsupported Mode ---------------------------------------------*/
#error "Invalid RF mode configuration of CC13xx device"
#endif /* CC13xx RF Mode configuration */

/*----- CC26xx Device Line --------------------------------------------------*/
/* CC26xx only supports IEEE mode */
#elif defined(DEVICE_LINE_CC26XX)

/* Default to IEEE-mode for CC26xx devices if not configured */
#ifndef RF_MODE
#define RF_MODE                      RF_MODE_2_4_GHZ
#endif

/*----- CC26xx IEEE-mode ----------------------------------------------------*/
#if (RF_MODE == RF_MODE_2_4_GHZ) && (SUPPORTS_IEEE_MODE)

/* Netstack configuration */
#define NETSTACK_CONF_RADIO         ieee_mode_driver

/* CSMA configuration */
#define CSMA_CONF_SEND_SOFT_ACK     0

/* Frequncy band configuration */
#undef DOT_15_4G_FREQ_BAND_ID
#define DOT_15_4G_CONF_FREQ_BAND_ID   DOT_15_4G_FREQ_BAND_2450

#else
/*----- CC26xx Unsupported Mode ---------------------------------------------*/
#error "IEEE-mode only supported by CC26xx devices"
#endif /* CC26xx RF Mode configuration */

/*----- Unsupported device line ---------------------------------------------*/
#else
#error "Unsupported Device Line defined"
#endif /* Unsupported device line */

/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name  IEEE address configuration. Used to generate our link-local and
 *        global IPv6 addresses.
 * @{
 */

/**
 * \brief  Location of the IEEE address.
 *         0 => Read from InfoPage.
 *         1 => Use a hardcoded address, configured by IEEE_ADDR_CONF_ADDRESS.
 */
#ifndef IEEE_ADDR_CONF_HARDCODED
#define IEEE_ADDR_CONF_HARDCODED            0
#endif

/**
 * \brief  The hardcoded IEEE address to be used when IEEE_ADDR_CONF_HARDCODED
 *         is defined as 1. Must be a byte array of size 8.
 */
#ifndef IEEE_ADDR_CONF_ADDRESS
#define IEEE_ADDR_CONF_ADDRESS              { 0x00, 0x12, 0x4B, 0x00, 0x89, 0xAB, 0xCD, 0xEF }
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name  IEEE-mode configuration.
 *
 * @{
 */

/**
 * \brief  Configuration to enable/disable auto ACKs in IEEE-mode.
 *         0 => ACK generated by software
 *         1 => ACK generated by the radio.
 */
#ifndef IEEE_MODE_CONF_AUTOACK
#define IEEE_MODE_CONF_AUTOACK              1
#endif

/**
 * \brief  Configuration to enable/disable frame filtering in IEEE-mode.
 *         0 => Disable promiscous mode.
 *         1 => Enable promiscous mode.
 */
#ifndef IEEE_MODE_CONF_PROMISCOUS
#define IEEE_MODE_CONF_PROMISCOUS           0
#endif

/**
 * \brief  Configuration to set the RSSI threshold in dBm in IEEE-mode.
 *         Defaults to -90 dBm.
 */
#ifndef IEEE_MODE_CONF_CCA_RSSI_THRESHOLD
#define IEEE_MODE_CONF_CCA_RSSI_THRESHOLD   0xA6
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name  Prop-mode configuration.
 *
 * @{
 */

/**
 * \brief  Configuration to set whitener in Prop-mode.
 *         0 => No whitener
 *         1 => Whitener.
 */
#ifndef PROP_MODE_CONF_DW
#define PROP_MODE_CONF_DW                   0
#endif

/**
 * \brief  Use 16-bit or 32-bit CRC in Prop-mode.
 *         0 => 32-bit CRC.
 *         1 => 16-bit CRC.
 */
#ifndef PROP_MODE_CONF_USE_CRC16
#define PROP_MODE_CONF_USE_CRC16            0
#endif

/**
 * \brief  Configuration to set the RSSI threshold in dBm in Prop-mode.
 *         Defaults to -90 dBm.
 */
#ifndef PROP_MODE_CONF_CCA_RSSI_THRESHOLD
#define PROP_MODE_CONF_CCA_RSSI_THRESHOLD   0xA6
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name  TI Drivers Configuration.
 *
 * @{
 */

/**
 * \brief  Enable or disable UART driver.
 */
#ifndef TI_UART_CONF_ENABLE
#define TI_UART_CONF_ENABLE                 1
#endif

/**
 * \brief  Enable or disable UART0 peripheral.
 */
#ifndef TI_UART_CONF_UART0_ENABLE
#define TI_UART_CONF_UART0_ENABLE           TI_UART_CONF_ENABLE
#endif

/**
 * \brief  Enable or disable UART1 peripheral.
 */
#ifndef TI_UART_CONF_UART1_ENABLE
#define TI_UART_CONF_UART1_ENABLE           0
#endif

/**
 * \brief  UART driver baud rate configuration.
 */
#ifndef TI_UART_CONF_BAUD_RATE
#define TI_UART_CONF_BAUD_RATE              115200
#endif

/**
 * \brief  Enable or disable SPI driver.
 */
#ifndef TI_SPI_CONF_ENABLE
#define TI_SPI_CONF_ENABLE                  1
#endif

/**
 * \brief  Enable or disable SPI0 peripheral.
 */
#ifndef TI_SPI_CONF_SPI0_ENABLE
#define TI_SPI_CONF_SPI0_ENABLE             TI_SPI_CONF_ENABLE
#endif

/**
 * \brief  Enable or disable SPI1 peripheral.
 */
#ifndef TI_SPI_CONF_SPI1_ENABLE
#define TI_SPI_CONF_SPI1_ENABLE             0
#endif

/**
 * \brief  Enable or disable I2C driver.
 */
#ifndef TI_I2C_CONF_ENABLE
#define TI_I2C_CONF_ENABLE                  1
#endif

/**
 * \brief  Enable or disable I2C0 peripheral.
 */
#ifndef TI_I2C_CONF_I2C0_ENABLE
#define TI_I2C_CONF_I2C0_ENABLE             TI_I2C_CONF_ENABLE
#endif

/**
 * \brief  Enable or disable Non-Volatile Storage (NVS) driver.
 */
#ifndef TI_NVS_CONF_ENABLE
#define TI_NVS_CONF_ENABLE                  0
#endif

/**
 * \brief  Enable or disable internal flash storage.
 */
#ifndef TI_NVS_CONF_NVS_INTERNAL_ENABLE
#define TI_NVS_CONF_NVS_INTERNAL_ENABLE     TI_NVS_CONF_ENABLE
#endif

/**
 * \brief  Enable or disable external flash storage.
 */
#ifndef TI_NVS_CONF_NVS_EXTERNAL_ENABLE
#define TI_NVS_CONF_NVS_EXTERNAL_ENABLE     TI_NVS_CONF_ENABLE
#endif

/**
 * \brief  Enable or disable SD driver.
 */
#ifndef TI_SD_CONF_ENABLE
#define TI_SD_CONF_ENABLE                   0
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name SPI HAL configuration.
 *
 * CC13x0/CC26x0 has one SPI interface, while CC13x2/CC26x2 has two
 * SPI interfaces. Some additional checks has to be made for the
 * SPI_CONF_CONTROLLER_COUNT configuration, as this relies on whether the
 * available SPI interfaces are enabled or not, as well as if the SPI driver
 * is enabled at all.
 *
 * @{
 */
#if TI_SPI_CONF_ENABLE
/*
 * The SPI driver is enabled. Therefore, the number of SPI interfaces depends
 * on which Device family parent the device falls under and if any of its
 * corresponding SPI interfaces are enabled or not.
 */

#define SPI0_IS_ENABLED                     ((TI_SPI_CONF_SPI0_ENABLE) ? 1 : 0)
#define SPI1_IS_ENABLED                     ((TI_SPI_CONF_SPI1_ENABLE) ? 1 : 0)

#if (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X0_CC26X0)

/* CC13x0/CC26x0 only has one SPI interface: SPI0. */
#define SPI_CONF_CONTROLLER_COUNT           (SPI0_IS_ENABLED)

#elif (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X2_CC26X2)

/* CC13x0/CC26x0 only has two SPI interface: SPI0 and SPI1. */
#define SPI_CONF_CONTROLLER_COUNT           (SPI0_IS_ENABLED + SPI1_IS_ENABLED)

#endif /* DeviceFamily_PARENT */

#else /* TI_SPI_CONF_ENABLE */
/*
 * If the SPI driver is disabled then there are 0 available
 * SPI interfaces. */
#define SPI_CONF_CONTROLLER_COUNT           0
#endif /* TI_SPI_CONF_ENABLE */
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Slip configuration
 *
 * @{
 */
#ifndef SLIP_ARCH_CONF_ENABLED
/*
 * Determine whether we need SLIP
 * This will keep working while UIP_FALLBACK_INTERFACE and CMD_CONF_OUTPUT
 * keep using SLIP
 */
#if defined(UIP_FALLBACK_INTERFACE) || defined(CMD_CONF_OUTPUT)
#define SLIP_ARCH_CONF_ENABLED              1
#endif

#endif /* SLIP_ARCH_CONF_ENABLED */
/** @} */
/*---------------------------------------------------------------------------*/
#endif /* CC13XX_CC26XX_CONF_H_ */
/*---------------------------------------------------------------------------*/
/** @} */

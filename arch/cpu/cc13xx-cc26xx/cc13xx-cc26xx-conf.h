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
#ifndef BOARD_CONF_HAS_SENSORS
#define BOARD_CONF_HAS_SENSORS              0
#endif

#ifndef BOARD_CONF_SENSORS_ENABLE
#define BOARD_CONF_SENSORS_ENABLE           BOARD_CONF_HAS_SENSORS
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
 * If set, the systems keeps the HF crystal oscillator on even when the
 * radio is off. You need to set this to 1 to use TSCH with its default 2.2ms
 * or larger guard time.
 */
#ifndef RF_CONF_FAST_RADIO_STARTUP
# define RF_FAST_RADIO_STARTUP        (MAC_CONF_WITH_TSCH)
#else
# define RF_FAST_RADIO_STARTUP        RF_CONF_FAST_RADIO_STARTUP
#endif

/*
 * Configure TX power to either default PA or High PA, defaults to
 * default PA.
 */
#ifndef RF_CONF_TXPOWER_HIGH_PA
#define RF_CONF_TXPOWER_HIGH_PA       0
#endif

#if (RF_CONF_TXPOWER_HIGH_PA) && !(SUPPORTS_HIGH_PA)
# error "Device does not support High PA"
#endif

/*
 * CC13xx only: Configure TX power to use boot mode, allowing to gain
 * up to 14 dBm with the default PA. This will, however, increase power
 * consumption.
 */
#ifndef RF_CONF_TXPOWER_BOOST_MODE
#define RF_CONF_TXPOWER_BOOST_MODE    0
#endif

/*
 * Configure RF mode. That is, whether to run on Sub-1 GHz (Prop-mode) or
 * 2.4 GHz (IEEE-mode).
 */
#ifdef RF_CONF_MODE
/* Sanity check a valid configuration is provided. */
# if !(RF_CONF_MODE & RF_MODE_BM)
#  error "Invalid RF_CONF_MODE provided"
# endif

# define RF_MODE                      RF_CONF_MODE
#endif /* RF_CONF_MODE */

/* Number of RX buffers. */
#ifdef RF_CONF_RX_BUF_CNT
# define RF_RX_BUF_CNT                RF_CONF_RX_BUF_CNT
#else
# define RF_RX_BUF_CNT                4
#endif

/* Size of each RX buffer in bytes. */
#ifdef RF_CONF_RX_BUF_SIZE
# define RF_RX_BUF_SIZE               RF_CONF_RX_BUF_SIZE
#else
# define RF_RX_BUF_SIZE               144
#endif

/* Enable/disable BLE beacon. */
#ifdef RF_CONF_BLE_BEACON_ENABLE
# define RF_BLE_BEACON_ENABLE         RF_CONF_BLE_BEACON_ENABLE
#else
# define RF_BLE_BEACON_ENABLE         0
#endif

#if (RF_BLE_BEACON_ENABLE) && !(SUPPORTS_BLE_BEACON)
# error "Device does not support BLE for BLE beacon"
#endif

/*----- CC13xx Device Line --------------------------------------------------*/
/* CC13xx supports both IEEE and Prop mode, depending on which device. */
#if defined(DEVICE_LINE_CC13XX)

/* Default to Prop-mode for CC13xx devices if not configured. */
# ifndef RF_MODE
# define RF_MODE                      RF_MODE_SUB_1_GHZ
# endif

/*----- CC13xx Prop-mode ----------------------------------------------------*/
# if (RF_MODE == RF_MODE_SUB_1_GHZ) && (SUPPORTS_PROP_MODE)

/* Netstack configuration. */
#  define NETSTACK_CONF_RADIO         prop_mode_driver

/* CSMA configuration. */
#  define CSMA_CONF_ACK_WAIT_TIME                (RTIMER_SECOND / 300)
#  define CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME (RTIMER_SECOND / 1000)
#  define CSMA_CONF_SEND_SOFT_ACK      1

/*----- CC13xx IEEE-mode ----------------------------------------------------*/
# elif (RF_MODE == RF_MODE_2_4_GHZ) && (SUPPORTS_IEEE_MODE)

/* Netstack configuration. */
#  define NETSTACK_CONF_RADIO         ieee_mode_driver

/* CSMA configuration. */
#  define CSMA_CONF_SEND_SOFT_ACK     0

# else
/*----- CC13xx Unsupported Mode ---------------------------------------------*/
#  error "Invalid RF mode configuration of CC13xx device"
# endif /* CC13xx RF Mode configuration */

/*----- CC26xx Device Line --------------------------------------------------*/
/* CC26xx only supports IEEE mode */
#elif defined(DEVICE_LINE_CC26XX)

/* Default to IEEE-mode for CC26xx devices if not configured */
# ifndef RF_MODE
# define RF_MODE                      RF_MODE_2_4_GHZ
# endif

/*----- CC26xx IEEE-mode ----------------------------------------------------*/
# if (RF_MODE == RF_MODE_2_4_GHZ) && (SUPPORTS_IEEE_MODE)

/* Netstack configuration */
#  define NETSTACK_CONF_RADIO         ieee_mode_driver

/* CSMA configuration */
#  define CSMA_CONF_SEND_SOFT_ACK     0

/* Frequncy band configuration */
#  undef DOT_15_4G_FREQ_BAND_ID
#  define DOT_15_4G_CONF_FREQ_BAND_ID   DOT_15_4G_FREQ_BAND_2450

# else
/*----- CC26xx Unsupported Mode ---------------------------------------------*/
#  error "IEEE-mode only supported by CC26xx devices"
# endif /* CC26xx RF Mode configuration */

/*----- Unsupported device line ---------------------------------------------*/
#else
# error "Unsupported Device Line defined"
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
 * \name IEEE-mode configuration.
 *
 * @{
 */

/**
 * \brief  Configure auto-ACK for IEEE-mode, which is ACK generated by the
 *         radio.
 *         0 => ACK generated by software
 *         1 => ACK generated by the radio.
 */
#ifndef IEEE_MODE_CONF_AUTOACK
#define IEEE_MODE_CONF_AUTOACK              1 /**< RF H/W generates ACKs */
#endif

/**
 * \brief  Configure promiscouos mode for IEEE-mode.
 */
#ifndef IEEE_MODE_CONF_PROMISCOUS
#define IEEE_MODE_CONF_PROMISCOUS           0 /**< 1 to enable promiscous mode */
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name TI Drivers Configuration.
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
#define TI_SPI_CONF_ENABLE                  0
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
#define TI_I2C_CONF_ENABLE                  0
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
 * \brief  Enable or disable Display driver.
 */
#ifndef TI_DISPLAY_CONF_ENABLE
#define TI_DISPLAY_CONF_ENABLE              0
#endif

/**
 * \brief  Enable or disable UART Display peripheral.
 */
#ifndef TI_DISPLAY_CONF_UART_ENABLE
#define TI_DISPLAY_CONF_UART_ENABLE         TI_UART_CONF_UART0_ENABLE
#endif

/**
 * \brief  Configure UART Display peripheral to use ANSI or not.
 */
#ifndef TI_DISPLAY_CONF_USE_UART_ANSI
#define TI_DISPLAY_CONF_USE_UART_ANSI       0
#endif

/**
 * \brief  Enable or disable LCD Display peripheral.
 */
#ifndef TI_DISPLAY_CONF_LCD_ENABLE
#define TI_DISPLAY_CONF_LCD_ENABLE          TI_SPI_CONF_SPI0_ENABLE
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

/*
 * Template:
 * Copyright (c) 2012 ARM LIMITED
 * All rights reserved.
 *
 * CC13xx-CC26xx:
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup cc26xx
 * @{
 *
 * \defgroup cc26xx-cm4 CC13xx/CC26xx CMSIS
 *
 * CC13xx/CC26xx Cortex-M4 CMSIS definitions
 * @{
 *
 * \file
 * CMSIS Cortex-M4 core peripheral access layer header file for CC13xx/CC26xx
 */
/*---------------------------------------------------------------------------*/
#ifndef CC13XX_CC26XX_CM4_H_
#define CC13XX_CC26XX_CM4_H_
/*---------------------------------------------------------------------------*/
/**
 * \name Interrupt Number Definition
 * @{
 */
typedef enum cc13xx_cc26xx_cm4_irq_e {
  /* Cortex-M4 Processor Exceptions */
  CC13XX_CC26XX_CM4_EXCEPTION_RESET = -15,            /**<  1 Reset */
  CC13XX_CC26XX_CM4_EXCEPTION_NMI = -14,              /**<  2 NMI */
  CC13XX_CC26XX_CM4_EXCEPTION_HARD_FAULT = -13,       /**<  3 Hard fault */
  CC13XX_CC26XX_CM4_EXCEPTION_MPU_FAULT = -12,        /**<  4 MPU fault */
  CC13XX_CC26XX_CM4_EXCEPTION_BUS_FAULT = -11,        /**<  5 Bus fault */
  CC13XX_CC26XX_CM4_EXCEPTION_USAGE_FAULT = -10,      /**<  6 Usage fault */
  CC13XX_CC26XX_CM4_EXCEPTION_SV_CALL = -5,           /**< 11 SVCall */
  CC13XX_CC26XX_CM4_EXCEPTION_DEBUG_MON = -4,         /**< 12 Debug monitor */
  CC13XX_CC26XX_CM4_EXCEPTION_PEND_SV = -2,           /**< 14 PendSV */
  CC13XX_CC26XX_CM4_EXCEPTION_SYS_TICK = -1,          /**< 15 SysTick */

  /* CC13xx/CC26xx interrupts */
  CC13XX_CC26XX_CM4_IRQ_EDGE_DETECT = 0,              /**< 16 AON edge detect */
  CC13XX_CC26XX_CM4_EXCEPTION_I2C = 1,                /**< 17 I2C */
  CC13XX_CC26XX_CM4_EXCEPTION_RF_CPE1 = 2,            /**< 18 RF Command and Packet Engine 1 */
  CC13XX_CC26XX_CM4_EXCEPTION_AON_SPI_SLAVE = 3,      /**< 19 AON SpiSplave Rx, Tx and CS */
  CC13XX_CC26XX_CM4_EXCEPTION_AON_RTC = 4,            /**< 20 AON RTC */
  CC13XX_CC26XX_CM4_EXCEPTION_UART0 = 5,              /**< 21 UART0 Rx and Tx */
  CC13XX_CC26XX_CM4_EXCEPTION_AON_AUX_SWEV0 = 6,      /**< 22 Sensor Controller software event 0, through AON domain*/
  CC13XX_CC26XX_CM4_EXCEPTION_SSI0 = 7,               /**< 23 SSI0 Rx and Tx */
  CC13XX_CC26XX_CM4_EXCEPTION_SSI1 = 8,               /**< 24 SSI1 Rx and Tx */
  CC13XX_CC26XX_CM4_EXCEPTION_RF_CPE0 = 9,            /**< 25 RF Command and Packet Engine 0 */
  CC13XX_CC26XX_CM4_EXCEPTION_RF_HW = 10,             /**< 26 RF Core Hardware */
  CC13XX_CC26XX_CM4_EXCEPTION_RF_CMD_ACK = 11,        /**< 27 RF Core Command Acknowledge */
  CC13XX_CC26XX_CM4_EXCEPTION_I2S = 12,               /**< 28 I2S */
  CC13XX_CC26XX_CM4_EXCEPTION_AON_AUX_SWEV1 = 13,     /**< 29 Sensor Controller software event 1, through AON domain*/
  CC13XX_CC26XX_CM4_EXCEPTION_WATCHDOG = 14,          /**< 30 Watchdog timer */
  CC13XX_CC26XX_CM4_EXCEPTION_GPTIMER_0A = 15,        /**< 31 Timer 0 subtimer A */
  CC13XX_CC26XX_CM4_EXCEPTION_GPTIMER_0B = 16,        /**< 32 Timer 0 subtimer B */
  CC13XX_CC26XX_CM4_EXCEPTION_GPTIMER_1A = 17,        /**< 33 Timer 1 subtimer A */
  CC13XX_CC26XX_CM4_EXCEPTION_GPTIMER_1B = 18,        /**< 34 Timer 1 subtimer B */
  CC13XX_CC26XX_CM4_EXCEPTION_GPTIMER_2A = 19,        /**< 35 Timer 2 subtimer A */
  CC13XX_CC26XX_CM4_EXCEPTION_GPTIMER_2B = 20,        /**< 36 Timer 2 subtimer B */
  CC13XX_CC26XX_CM4_EXCEPTION_GPTIMER_3A = 21,        /**< 37 Timer 3 subtimer A */
  CC13XX_CC26XX_CM4_EXCEPTION_GPTIMER_3B = 22,        /**< 38 Timer 3 subtimer B */
  CC13XX_CC26XX_CM4_EXCEPTION_CRYPTO = 23,            /**< 39 Crypto Core Result available */
  CC13XX_CC26XX_CM4_EXCEPTION_UDMA = 24,              /**< 40 uDMA Software */
  CC13XX_CC26XX_CM4_EXCEPTION_UDMA_ERR = 25,          /**< 41 uDMA Error */
  CC13XX_CC26XX_CM4_EXCEPTION_FLASH_CTRL = 26,        /**< 42 Flash controller */
  CC13XX_CC26XX_CM4_EXCEPTION_SW0 = 27,               /**< 43 Software Event 0 */
  CC13XX_CC26XX_CM4_EXCEPTION_AUX_COM_EVENT = 28,     /**< 44 AUX combined event, directly to MCU domain*/
  CC13XX_CC26XX_CM4_EXCEPTION_AON_PRG0 = 29,          /**< 45 AON programmable 0 */
  CC13XX_CC26XX_CM4_EXCEPTION_PROG = 30,              /**< 46 Dynamic Programmable interrupt (default source: PRCM)*/
  CC13XX_CC26XX_CM4_EXCEPTION_AUX_COMPA = 31,         /**< 47 AUX Comparator A */
  CC13XX_CC26XX_CM4_EXCEPTION_AUX_ADC = 32,           /**< 48 AUX ADC IRQ */
  CC13XX_CC26XX_CM4_EXCEPTION_TRNG = 33,              /**< 49 TRNG event */
} cc13xx_cc26xx_cm4_irq_t;

typedef cc13xx_cc26xx_cm4_irq_t IRQn_Type;

#define SysTick_IRQn CC13XX_CC26XX_CM4_EXCEPTION_SYS_TICK
/** @} */
/*---------------------------------------------------------------------------*/
/** \name Processor and Core Peripheral Section
 * @{
 */
/* Configuration of the Cortex-M4 Processor and Core Peripherals */
#define __MPU_PRESENT           1       /**< MPU present or not */
#define __NVIC_PRIO_BITS        3       /**< Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig  0       /**< Set to 1 if different SysTick Config is used */
/** @} */
/*---------------------------------------------------------------------------*/
#include "core_cm4.h" /* Cortex-M4 processor and core peripherals */
/*---------------------------------------------------------------------------*/
#endif /* CC13XX_CC26XX_CM4_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

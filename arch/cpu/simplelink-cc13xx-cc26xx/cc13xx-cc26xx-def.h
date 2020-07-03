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
 *        Header with configuration defines for the Contiki system.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef CC13XX_CC26XX_DEF_H_
#define CC13XX_CC26XX_DEF_H_
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
/*---------------------------------------------------------------------------*/
#if (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X0_CC26X0)
#include <cm3/cm3-def.h>
#elif (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X2_CC26X2)
#include <cm4/cm4-def.h>
#endif
/*---------------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define RTIMER_ARCH_SECOND               65536
/*---------------------------------------------------------------------------*/
#define INT_MASTER_CONF_STATUS_DATATYPE  uintptr_t
/*---------------------------------------------------------------------------*/
/* TSCH related defines */

/* 2 bytes header, 4 bytes CRC */
#define CC13XX_RADIO_PHY_OVERHEAD 6
/* 3 bytes preamble, 3 bytes sync */
#define CC13XX_RADIO_PHY_HEADER_LEN 6
/* The default data rate is 50 kbps */
#define CC13XX_RADIO_BIT_RATE 50000

/* 1 len byte, 2 bytes CRC */
#define CC26XX_RADIO_PHY_OVERHEAD     3
/* 4 bytes preamble, 1 byte sync */
#define CC26XX_RADIO_PHY_HEADER_LEN   5
/* The fixed data rate is 250 kbps */
#define CC26XX_RADIO_BIT_RATE  250000

#if defined(DEVICE_LINE_CC13XX)
#define RADIO_PHY_HEADER_LEN CC13XX_RADIO_PHY_HEADER_LEN
#define RADIO_PHY_OVERHEAD   CC13XX_RADIO_PHY_OVERHEAD
#define RADIO_BIT_RATE       CC13XX_RADIO_BIT_RATE

/* The TSCH default slot length of 10ms is too short, use custom one instead */
#ifndef TSCH_CONF_DEFAULT_TIMESLOT_TIMING
#define TSCH_CONF_DEFAULT_TIMESLOT_TIMING tsch_timing_cc13xx_50kbps
#endif /* TSCH_CONF_DEFAULT_TIMESLOT_TIMING */

/* Symbol for the custom TSCH timeslot timing template */
#define TSCH_CONF_ARCH_HDR_PATH "rf/cc13xx-50kbps-tsch.h"

#else
#define RADIO_PHY_HEADER_LEN CC26XX_RADIO_PHY_HEADER_LEN
#define RADIO_PHY_OVERHEAD   CC26XX_RADIO_PHY_OVERHEAD
#define RADIO_BIT_RATE       CC26XX_RADIO_BIT_RATE
#endif

#define RADIO_BYTE_AIR_TIME  (1000000 / (RADIO_BIT_RATE / 8))

/* Delay between GO signal and SFD */
#define RADIO_DELAY_BEFORE_TX ((unsigned)US_TO_RTIMERTICKS(RADIO_PHY_HEADER_LEN * RADIO_BYTE_AIR_TIME))
/* Delay between GO signal and start listening.
 * This value is so small because the radio is constantly on within each timeslot. */
#define RADIO_DELAY_BEFORE_RX       ((unsigned)US_TO_RTIMERTICKS(15))
/* Delay between the SFD finishes arriving and it is detected in software. */
#define RADIO_DELAY_BEFORE_DETECT   ((unsigned)US_TO_RTIMERTICKS(352))

/* Timer conversion; radio is running at 4 MHz */
#define RAT_SECOND            4000000u
#define RAT_TO_RTIMER(x)      ((uint32_t)(((uint64_t)(x) * (RTIMER_SECOND / 256)) / (RAT_SECOND / 256)))
#define USEC_TO_RAT(x)        ((x) * 4)

#if (RTIMER_SECOND % 256) || (RAT_SECOND % 256)
#error RAT_TO_RTIMER macro must be fixed!
#endif

/* Do not turn off TSCH within a timeslot: not enough time */
#define TSCH_CONF_RADIO_ON_DURING_TIMESLOT 1

/* Disable TSCH frame filtering */
#define TSCH_CONF_HW_FRAME_FILTERING  0

/* Use hardware timestamps */
#ifndef TSCH_CONF_RESYNC_WITH_SFD_TIMESTAMPS
#define TSCH_CONF_RESYNC_WITH_SFD_TIMESTAMPS 1
#define TSCH_CONF_TIMESYNC_REMOVE_JITTER 0
#endif

#ifndef TSCH_CONF_BASE_DRIFT_PPM
#if defined(DEVICE_LINE_CC13XX)
/* The drift compared to "true" 40ms slots.
 * Enable adaptive sync to enable compensation for this.
 * Slot length 40000 usec
 *             1311 ticks
 * Tick duration 30.517578125 usec
 * Real slot duration 40008.544922 usec
 * Target - real duration = -8.544922 usec
 * PPM -8.544922 / 40000 = -0.000214
 */
#define TSCH_CONF_BASE_DRIFT_PPM -214
#else /* defined(DEVICE_LINE_CC13XX) */
/*
 * The drift compared to "true" 10ms slots.
 * Enable adaptive sync to enable compensation for this.
 * Slot length 10000 usec
 *             328 ticks
 * Tick duration 30.517578125 usec
 * Real slot duration 10009.765625 usec
 * Target - real duration = -9.765625 usec
 * TSCH_CONF_BASE_DRIFT_PPM -977
 */
#define TSCH_CONF_BASE_DRIFT_PPM -977
#endif /* defined(DEVICE_LINE_CC13XX) */
#endif /* TSCH_CONF_BASE_DRIFT_PPM */

/* 10 times per second */
#ifndef TSCH_CONF_CHANNEL_SCAN_DURATION
#define TSCH_CONF_CHANNEL_SCAN_DURATION (CLOCK_SECOND / 10)
#endif

/* Tweak to improve TSCH association speed on this platform */
#ifndef TSCH_CONF_ASSOCIATION_POLL_FREQUENCY
#define TSCH_CONF_ASSOCIATION_POLL_FREQUENCY 10
#endif

/* Slightly reduce the TSCH guard time (from 2200 usec to 1800 usec) to make sure
 * the CC26xx radio has sufficient time to start up. */
#ifndef TSCH_CONF_RX_WAIT
#define TSCH_CONF_RX_WAIT 1800
#endif
/*---------------------------------------------------------------------------*/
/* Path to CMSIS header */
#if (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X0_CC26X0)
#define CMSIS_CONF_HEADER_PATH              "cc13x0-cc26x0-cm3.h"
#elif (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X2_CC26X2)
#define CMSIS_CONF_HEADER_PATH              "cc13x2-cc26x2-cm4.h"
#endif
/*---------------------------------------------------------------------------*/
/* Path to headers with implementation of mutexes, atomic and memory barriers */
#define MUTEX_CONF_ARCH_HEADER_PATH          "mutex-cortex.h"
#define ATOMIC_CONF_ARCH_HEADER_PATH         "atomic-cortex.h"
#define MEMORY_BARRIER_CONF_ARCH_HEADER_PATH "memory-barrier-cortex.h"
/*---------------------------------------------------------------------------*/
/* GPIO HAL configuration */
#define GPIO_HAL_CONF_ARCH_HDR_PATH         "dev/gpio-hal-arch.h"
#define GPIO_HAL_CONF_ARCH_SW_TOGGLE        0
#define GPIO_HAL_CONF_PORT_PIN_NUMBERING    0
/*---------------------------------------------------------------------------*/
#endif /* CC13XX_CC26XX_DEF_H_ */
/*---------------------------------------------------------------------------*/
/** @} */

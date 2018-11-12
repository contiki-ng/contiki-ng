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
 * \addtogroup cc13xx-cc26xx-rf
 * @{
 *
 * \defgroup cc13xx-cc26xx-rf-15-4g-modes IEEE 802.15.4g Frequency Bands and Modes
 *
 * @{
 *
 * \file
 *        Header file with descriptors for the various modes of operation
 *        defined in IEEE 802.15.4g.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef DOT_15_4G_H_
#define DOT_15_4G_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
/* IEEE 802.15.4g frequency band identifiers (Table 68f) */
#define DOT_15_4G_FREQ_BAND_169     0 /* 169.400–169.475 (Europe) - 169 MHz band */
#define DOT_15_4G_FREQ_BAND_450     1 /* 450–470 (US FCC Part 22/90) - 450 MHz band */
#define DOT_15_4G_FREQ_BAND_470     2 /* 470–510 (China) - 470 MHz band */
#define DOT_15_4G_FREQ_BAND_780     3 /* 779–787 (China) - 780 MHz band */
#define DOT_15_4G_FREQ_BAND_863     4 /* 863–870 (Europe) - 863 MHz band */
#define DOT_15_4G_FREQ_BAND_896     5 /* 896–901 (US FCC Part 90) - 896 MHz band */
#define DOT_15_4G_FREQ_BAND_901     6 /* 901–902 (US FCC Part 24) - 901 MHz band */
#define DOT_15_4G_FREQ_BAND_915     7 /* 902–928 (US) - 915 MHz band */
#define DOT_15_4G_FREQ_BAND_917     8 /* 917–923.5 (Korea) - 917 MHz band */
#define DOT_15_4G_FREQ_BAND_920     9 /* 920–928 (Japan) - 920 MHz band */
#define DOT_15_4G_FREQ_BAND_928    10 /* 928–960 (US, non-contiguous) - 928 MHz band */
#define DOT_15_4G_FREQ_BAND_950    11 /* 950–958 (Japan) - 950 MHz band */
#define DOT_15_4G_FREQ_BAND_1427   12 /* 1427–1518 (US and Canada, non-contiguous) - 1427 MHz band */
#define DOT_15_4G_FREQ_BAND_2450   13 /* 2400–2483.5 2450 MHz band */
/*---------------------------------------------------------------------------*/
/* Default band selection to band 4 - 863MHz */
#ifdef DOT_15_4G_CONF_FREQ_BAND_ID
#define DOT_15_4G_FREQ_BAND_ID    DOT_15_4G_CONF_FREQ_BAND_ID
#else
#define DOT_15_4G_FREQ_BAND_ID    DOT_15_4G_FREQ_BAND_863
#endif
/*---------------------------------------------------------------------------*/
/*
 * Channel count, spacing and other params relating to the selected band. We
 * currently only support some of the bands defined in .15.4g and for those
 * bands we only support operating mode #1 (Table 134).
 *
 * DOT_15_4G_CHAN0_FREQ is specified here in KHz
 */
#if (DOT_15_4G_FREQ_BAND_ID == DOT_15_4G_FREQ_BAND_470)
#define DOT_15_4G_CHAN_MIN           0
#define DOT_15_4G_CHAN_MAX           198
#define DOT_15_4G_FREQ_SPACING       200
#define DOT_15_4G_CHAN0_FREQ         470200

#define PROP_MODE_CONF_CENTER_FREQ   0x01EA
#define PROP_MODE_CONF_LO_DIVIDER    0x0A

#elif (DOT_15_4G_FREQ_BAND_ID == DOT_15_4G_FREQ_BAND_780)
#define DOT_15_4G_CHAN_MIN           0
#define DOT_15_4G_CHAN_MAX           38
#define DOT_15_4G_FREQ_SPACING       200
#define DOT_15_4G_CHAN0_FREQ         779200

#define PROP_MODE_CONF_CENTER_FREQ   0x030F
#define PROP_MODE_CONF_LO_DIVIDER    0x06

#elif (DOT_15_4G_FREQ_BAND_ID == DOT_15_4G_FREQ_BAND_863)
#define DOT_15_4G_CHAN_MIN           0
#define DOT_15_4G_CHAN_MAX           33
#define DOT_15_4G_FREQ_SPACING       200
#define DOT_15_4G_CHAN0_FREQ         863125

#define PROP_MODE_CONF_CENTER_FREQ   0x0362
#define PROP_MODE_CONF_LO_DIVIDER    0x05

#elif (DOT_15_4G_FREQ_BAND_ID == DOT_15_4G_FREQ_BAND_915)
#define DOT_15_4G_CHAN_MIN           0
#define DOT_15_4G_CHAN_MAX           128
#define DOT_15_4G_FREQ_SPACING       200
#define DOT_15_4G_CHAN0_FREQ         902200

#define PROP_MODE_CONF_CENTER_FREQ   0x0393
#define PROP_MODE_CONF_LO_DIVIDER    0x05

#elif (DOT_15_4G_FREQ_BAND_ID == DOT_15_4G_FREQ_BAND_920)
#define DOT_15_4G_CHAN_MIN           0
#define DOT_15_4G_CHAN_MAX           37
#define DOT_15_4G_FREQ_SPACING       200
#define DOT_15_4G_CHAN0_FREQ         920600

#define PROP_MODE_CONF_CENTER_FREQ   0x039C
#define PROP_MODE_CONF_LO_DIVIDER    0x05

#elif (DOT_15_4G_FREQ_BAND_ID == DOT_15_4G_FREQ_BAND_950)
#define DOT_15_4G_CHAN_MIN           0
#define DOT_15_4G_CHAN_MAX           32
#define DOT_15_4G_FREQ_SPACING       200
#define DOT_15_4G_CHAN0_FREQ         951000

#define PROP_MODE_CONF_CENTER_FREQ   0x03BA
#define PROP_MODE_CONF_LO_DIVIDER    0x05

#elif (DOT_15_4G_FREQ_BAND_ID == DOT_15_4G_FREQ_BAND_2450)
#define DOT_15_4G_CHAN_MIN           11
#define DOT_15_4G_CHAN_MAX           26
#define DOT_15_4G_FREQ_SPACING       5000
#define DOT_15_4G_CHAN0_FREQ         2405000

#else
#error "The selected IEEE 802.15.4g frequency band is not supported"
#endif
/*---------------------------------------------------------------------------*/
static inline uint32_t
dot_15_4g_freq(const uint16_t chan)
{
  const uint32_t chan0 = DOT_15_4G_CHAN0_FREQ;
  const uint32_t spacing = DOT_15_4G_FREQ_SPACING;
  const uint32_t chan_min = DOT_15_4G_CHAN_MIN;
  return chan0 + spacing * ((uint32_t)chan - chan_min);
}
/*---------------------------------------------------------------------------*/
static inline bool
dot_15_4g_chan_in_range(const uint16_t chan)
{
  const uint16_t chan_min = DOT_15_4G_CHAN_MIN;
  const uint16_t chan_max = DOT_15_4G_CHAN_MAX;
  return (chan >= chan_min) &&
         (chan <= chan_max);
}
/*---------------------------------------------------------------------------*/
#define DOT_15_4G_DEFAULT_CHAN      IEEE802154_DEFAULT_CHANNEL
/*---------------------------------------------------------------------------*/
#endif /* DOT_15_4G_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

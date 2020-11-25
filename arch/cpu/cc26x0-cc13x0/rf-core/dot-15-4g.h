/*
 * Copyright (c) 2015, Texas Instruments Incorporated - http://www.ti.com/
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup rf-core
 * @{
 *
 * \defgroup rf-core-15-4g-modes IEEE 802.15.4g Frequency Bands and Modes
 *
 * @{
 *
 * \file
 * Header file with descriptors for the various modes of operation defined in
 * IEEE 802.15.4g
 */
/*---------------------------------------------------------------------------*/
#ifndef DOT_15_4G_H_
#define DOT_15_4G_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "driverlib/rf_mailbox.h"
/*---------------------------------------------------------------------------*/
/* IEEE 802.15.4g frequency band identifiers (Table 68f) */
#define DOT_15_4G_FREQUENCY_BAND_169     0 /* 169.400–169.475 (Europe) - 169 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_450     1 /* 450–470 (US FCC Part 22/90) - 450 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_470     2 /* 470–510 (China) - 470 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_780     3 /* 779–787 (China) - 780 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_863     4 /* 863–870 (Europe) - 863 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_896     5 /* 896–901 (US FCC Part 90) - 896 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_901     6 /* 901–902 (US FCC Part 24) - 901 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_915     7 /* 902–928 (US) - 915 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_917     8 /* 917–923.5 (Korea) - 917 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_920     9 /* 920–928 (Japan) - 920 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_928     10 /* 928–960 (US, non-contiguous) - 928 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_950     11 /* 950–958 (Japan) - 950 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_1427    12 /* 1427–1518 (US and Canada, non-contiguous) - 1427 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_2450    13 /* 2400–2483.5 2450 MHz band */
#define DOT_15_4G_FREQUENCY_BAND_CUSTOM  14 /* For use with custom frequency band settings */
#define DOT_15_4G_FREQUENCY_BAND_431     15 /* 431–527 - 433 MHz band */
/*---------------------------------------------------------------------------*/
/* Default band selection to band 4 - 863MHz */
#ifdef DOT_15_4G_CONF_FREQUENCY_BAND_ID
#define DOT_15_4G_FREQUENCY_BAND_ID DOT_15_4G_CONF_FREQUENCY_BAND_ID
#else
#define DOT_15_4G_FREQUENCY_BAND_ID DOT_15_4G_FREQUENCY_BAND_863
#endif
/*---------------------------------------------------------------------------*/
/*
 * Channel count, spacing and other params relating to the selected band. We
 * currently only support some of the bands defined in .15.4g and for those
 * bands we only support operating mode #1 (Table 134).
 *
 * DOT_15_4G_CHAN0_FREQUENCY is specified here in KHz
 *
 * Custom bands and configuration can be used with DOT_15_4G_FREQUENCY_BAND_CUSTOM.
 *
 * Example of custom setup for the 868Mhz sub-band in Europe with 11 channels,
 * center frequency at 868.050MHz and channel spacing at 100KHz.
 * These should be put in project-config.h or similar.
 *
 * #define DOT_15_4G_FREQUENCY_BAND_ID DOT_15_4G_FREQUENCY_BAND_CUSTOM
 * #define DOT_15_4G_CHAN0_FREQUENCY        868050
 * #define DOT_15_4G_CHANNEL_SPACING        100
 * #define DOT_15_4G_CHANNEL_MAX            11
 * #define PROP_MODE_CONF_LO_DIVIDER        0x05
 */
#if DOT_15_4G_FREQUENCY_BAND_ID==DOT_15_4G_FREQUENCY_BAND_470
#define DOT_15_4G_CHANNEL_MAX        198
#define DOT_15_4G_CHANNEL_SPACING    200
#define DOT_15_4G_CHAN0_FREQUENCY 470200
#define PROP_MODE_CONF_LO_DIVIDER   0x0A
#define SMARTRF_SETTINGS_CONF_BAND_OVERRIDES HW32_ARRAY_OVERRIDE(0x405C,1), \
                                             (uint32_t)0x18000280,

#elif DOT_15_4G_FREQUENCY_BAND_ID==DOT_15_4G_FREQUENCY_BAND_431
#define DOT_15_4G_CHANNEL_MAX        479
#define DOT_15_4G_CHANNEL_SPACING    200
#define DOT_15_4G_CHAN0_FREQUENCY 431120
#define PROP_MODE_CONF_LO_DIVIDER   0x0A

#elif DOT_15_4G_FREQUENCY_BAND_ID==DOT_15_4G_FREQUENCY_BAND_780
#define DOT_15_4G_CHANNEL_MAX         38
#define DOT_15_4G_CHANNEL_SPACING    200
#define DOT_15_4G_CHAN0_FREQUENCY 779200
#define PROP_MODE_CONF_LO_DIVIDER   0x06

#elif DOT_15_4G_FREQUENCY_BAND_ID==DOT_15_4G_FREQUENCY_BAND_863
#define DOT_15_4G_CHANNEL_MAX         33
#define DOT_15_4G_CHANNEL_SPACING    200
#define DOT_15_4G_CHAN0_FREQUENCY 863125
#define PROP_MODE_CONF_LO_DIVIDER   0x05

#elif DOT_15_4G_FREQUENCY_BAND_ID==DOT_15_4G_FREQUENCY_BAND_915
#define DOT_15_4G_CHANNEL_MAX        128
#define DOT_15_4G_CHANNEL_SPACING    200
#define DOT_15_4G_CHAN0_FREQUENCY 902200
#define PROP_MODE_CONF_LO_DIVIDER   0x05

#elif DOT_15_4G_FREQUENCY_BAND_ID==DOT_15_4G_FREQUENCY_BAND_920
#define DOT_15_4G_CHANNEL_MAX         37
#define DOT_15_4G_CHANNEL_SPACING    200
#define DOT_15_4G_CHAN0_FREQUENCY 920600
#define PROP_MODE_CONF_LO_DIVIDER   0x05

#elif DOT_15_4G_FREQUENCY_BAND_ID==DOT_15_4G_FREQUENCY_BAND_950
#define DOT_15_4G_CHANNEL_MAX         32
#define DOT_15_4G_CHANNEL_SPACING    200
#define DOT_15_4G_CHAN0_FREQUENCY 951000
#define PROP_MODE_CONF_LO_DIVIDER   0x05

#elif (DOT_15_4G_FREQ_BAND_ID == DOT_15_4G_FREQ_BAND_2450)
#define DOT_15_4G_CHANNEL_MIN        11
#define DOT_15_4G_CHANNEL_MAX        26
#define DOT_15_4G_FREQ_SPACING       5000
#define DOT_15_4G_CHAN0_FREQ         2405000

#elif DOT_15_4G_FREQUENCY_BAND_ID==DOT_15_4G_FREQUENCY_BAND_CUSTOM
#ifndef DOT_15_4G_CHANNEL_MAX
#error DOT_15_4G_CHANNEL_MAX must be manually set when using custom frequency band
#endif

#ifndef DOT_15_4G_CHANNEL_SPACING
#error DOT_15_4G_CHANNEL_SPACING must be manually set when using custom frequency band
#endif

#ifndef DOT_15_4G_CHAN0_FREQUENCY
#error DOT_15_4G_CHAN0_FREQUENCY must be manually set when using custom frequency band
#endif

#ifndef PROP_MODE_CONF_LO_DIVIDER
#error PROP_MODE_CONF_LO_DIVIDER must be manually set when using custom frequency band
#endif
#else
#error The selected frequency band is not supported
#endif

#ifndef DOT_15_4G_CHANNEL_MIN
#define DOT_15_4G_CHANNEL_MIN       0
#endif


//==============================================================================
//          legacy compatibily with simplelink target

#define DOT_15_4G_FREQ_BAND_169     DOT_15_4G_FREQUENCY_BAND_169
#define DOT_15_4G_FREQ_BAND_431     DOT_15_4G_FREQUENCY_BAND_431
#define DOT_15_4G_FREQ_BAND_450     DOT_15_4G_FREQUENCY_BAND_450
#define DOT_15_4G_FREQ_BAND_470     DOT_15_4G_FREQUENCY_BAND_470
#define DOT_15_4G_FREQ_BAND_780     DOT_15_4G_FREQUENCY_BAND_780
#define DOT_15_4G_FREQ_BAND_863     DOT_15_4G_FREQUENCY_BAND_863
#define DOT_15_4G_FREQ_BAND_896     DOT_15_4G_FREQUENCY_BAND_896
#define DOT_15_4G_FREQ_BAND_901     DOT_15_4G_FREQUENCY_BAND_901
#define DOT_15_4G_FREQ_BAND_915     DOT_15_4G_FREQUENCY_BAND_915
#define DOT_15_4G_FREQ_BAND_917     DOT_15_4G_FREQUENCY_BAND_917
#define DOT_15_4G_FREQ_BAND_920     DOT_15_4G_FREQUENCY_BAND_920
#define DOT_15_4G_FREQ_BAND_928     DOT_15_4G_FREQUENCY_BAND_928
#define DOT_15_4G_FREQ_BAND_950     DOT_15_4G_FREQUENCY_BAND_950
#define DOT_15_4G_FREQ_BAND_1427    DOT_15_4G_FREQUENCY_BAND_1427
#define DOT_15_4G_FREQ_BAND_2450    DOT_15_4G_FREQUENCY_BAND_2450
#define DOT_15_4G_FREQ_BAND_CUSTOM  DOT_15_4G_FREQUENCY_BAND_CUSTOM

#define DOT_15_4G_CHAN0_FREQ        DOT_15_4G_CHAN0_FREQUENCY
#define DOT_15_4G_CHAN_MIN          DOT_15_4G_CHANNEL_MIN
#define DOT_15_4G_CHAN_MAX          DOT_15_4G_CHANNEL_MAX
#define DOT_15_4G_FREQ_SPACING      DOT_15_4G_CHANNEL_SPACING
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

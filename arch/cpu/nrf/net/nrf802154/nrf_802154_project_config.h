/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
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
 *
 * nrf_802154 library configuration for the Contiki-NG nRF5340
 * network-core radio port. Adapted from the nRF54L15 port
 * (arch/cpu/nrf/nrf54l15/nrf_802154_project_config.h).
 *
 * Peripheral instances are chosen to avoid the ones Contiki-NG already
 * claims on the net core:
 *   - System clock  -> RTC0   (clock-arch.c, NRF_CLOCK_CONF_RTC_INSTANCE=0)
 *   - rtimer        -> TIMER0  (rtimer-arch.c, TIMER_INSTANCE default 0)
 *   - IPC transport -> IPC peripheral + IPC_IRQ (no EGU/DPPI)
 *
 * The nrf_802154 nRF53 defaults (nrf_802154_peripherals_nrf53.h +
 * nrf_802154_peripherals.h) are: EGU0, RTC2, DPPIC, TIMER0. RTC2 and
 * EGU0 are free, but the library's TIMER0 collides with rtimer, so the
 * radio TIMER is moved below. (Alternatively, relocate rtimer instead
 * via NRF_RTIMER_CONF_TIMER_INSTANCE in the example's project-conf.h.)
 */

#ifndef NRF_802154_PROJECT_CONFIG_H_
#define NRF_802154_PROJECT_CONFIG_H_

/* ---- Peripheral instance assignments (avoid Contiki-NG's) ------------- */

/* Radio fine-timing / ACK-IFS timer. Default is TIMER0 (used by rtimer),
 * so move the library to TIMER1. */
#define NRF_802154_TIMER_INSTANCE_NO                1
/* High-precision (timestamp) timer; unused while timestamping is off, but
 * reserve TIMER2 so NRF_802154_TIMERS_USED_MASK stays off TIMER0. */
#define NRF_802154_HIGH_PRECISION_TIMER_INSTANCE_NO 2

/* The nRF5340 NETWORK core has only RTC0 and RTC1 -- there is no RTC2, so
 * the nrf53 header default (RTC_INSTANCE_NO=2) does not exist here. RTC0 is
 * the Contiki-NG system clock, so the low-power timer backend uses RTC1.
 * (Build must NOT enable NRFX_RTC1 so this file owns RTC1_IRQHandler.) */
#define NRF_802154_RTC_INSTANCE_NO                  1

/* EGU0 (library default) is unused by Contiki-NG on the net core. The
 * library's DPPIC channels are likewise free: the only other nrfx_gppi
 * user on this core, the legacy nrf-ieee-driver-arch.c, is excluded from
 * the build when NRF_802154=1 (see nrf802154.mk). */

/* ---- IRQ ownership ---------------------------------------------------- */

/* The library owns the net core's RADIO IRQ directly. Use direct (non-SWI)
 * notification/request as the nRF54L15 port does, to avoid wiring an EGU
 * IRQ handler in the platform layer. */
#define NRF_802154_INTERNAL_RADIO_IRQ_HANDLING      1
#define NRF_802154_INTERNAL_SWI_IRQ_HANDLING        0
#define NRF_802154_NOTIFICATION_IMPL                NRF_802154_NOTIFICATION_IMPL_DIRECT
#define NRF_802154_REQUEST_IMPL                     NRF_802154_REQUEST_IMPL_DIRECT

/* ---- Feature set ------------------------------------------------------ */

/* Auto-ACK is what this port is for: the wrapper calls
 * nrf_802154_auto_ack_set(true); the library generates the immediate ACK
 * in hardware within the 802.15.4 ACK window (fixing the CC2538 interop).
 * Contiki-NG CSMA on the app core still does TX backoff, so disable the
 * library's own CSMA. Keep TX-side ACK timeout. */
#define NRF_802154_CSMA_CA_ENABLED                  0
#define NRF_802154_ACK_TIMEOUT_ENABLED              1
#define NRF_802154_IFS_ENABLED                      0
#define NRF_802154_DELAYED_TRX_ENABLED              0      /* no TSCH yet */

/* Open-source SL feature trim (matches the nRF54L15 port). */
#define NRF_802154_ENCRYPTION_ENABLED               0
#define NRF_802154_IE_WRITER_ENABLED                0
#define NRF_802154_SECURITY_WRITER_ENABLED          0
#define NRF_802154_CARRIER_FUNCTIONS_ENABLED        0
#define NRF_802154_TEST_MODES_ENABLED               0
#define NRF_802154_FRAME_TIMESTAMP_ENABLED          0
#define NRF_802154_NOTIFY_CRCERROR                  0
#define NRF_802154_SERIALIZATION_HOST               0

/* ---- CCA / buffers (nRF54L15 values; re-tune on nRF53 hardware) ------- */
#define NRF_802154_CCA_MODE_DEFAULT                 NRF_RADIO_CCA_MODE_ED
#define NRF_802154_CCA_ED_THRESHOLD_DBM_DEFAULT     (-75)
#define NRF_802154_CCA_CORR_THRESHOLD_DEFAULT       45
#define NRF_802154_CCA_CORR_LIMIT_DEFAULT           2
#define NRF_802154_PENDING_SHORT_ADDRESSES          16
#define NRF_802154_PENDING_EXTENDED_ADDRESSES       16
#define NRF_802154_RX_BUFFERS                       20

/* Extra ACK turnaround slack for the bare-metal Contiki-NG timer backend.
 * Start at 0 on nRF53 and only raise if RX-side ACKs land late. */
#define NRF_802154_ACK_IFS_EXTRA_TIME_US            0U

/* No closed-source SL: must remain undefined, not 0. */
#ifdef NRF_802154_USE_INTERNAL_INCLUDES
#undef NRF_802154_USE_INTERNAL_INCLUDES
#endif

#endif /* NRF_802154_PROJECT_CONFIG_H_ */

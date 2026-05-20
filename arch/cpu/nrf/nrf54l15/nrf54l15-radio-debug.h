/*
 * Copyright (c) 2026, Contiki-NG.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NRF54L15_RADIO_DEBUG_H_
#define NRF54L15_RADIO_DEBUG_H_

#include <stdint.h>

#define NRF54L15_BAD_ACK_NONE        0U
#define NRF54L15_BAD_ACK_CRC_ERROR   1U
#define NRF54L15_BAD_ACK_PARSE_FAIL  2U
#define NRF54L15_BAD_ACK_MISMATCH    3U

typedef struct {
  volatile uint32_t rx_frame_ok;
  volatile uint32_t ack_requested;
  volatile uint32_t ack_tx_arm_ok;
  volatile uint32_t ack_tx_arm_fail;
  volatile uint32_t ack_tx_started;
  volatile uint32_t ack_tx_done;
  volatile uint32_t ack_rx_started;
  volatile uint32_t ack_rx_valid;
  volatile uint32_t ack_rx_invalid;
  volatile uint32_t ack_timeout;
  volatile uint32_t ack_timeout_timer_start;
  volatile uint32_t ack_timeout_timer_stop;
  volatile uint32_t ack_timeout_timer_fire;
  volatile uint32_t ack_timeout_timer_retry;
  volatile uint32_t ack_timeout_req_accepted;
  volatile uint32_t ack_timeout_req_rejected;
  volatile uint32_t ack_delay_too_short;
  volatile uint32_t sl_timer_alarm_callback;
  volatile uint32_t sl_timer_alarm_pending_seen;
  volatile uint32_t sl_timer_sync_pending_seen;
  volatile uint32_t sl_timer_alarm_ccen_fix;
  volatile uint32_t sl_timer_sync_ccen_fix;
  volatile uint32_t sl_timer_critical_depth;
  volatile uint32_t sl_timer_critical_max;
  volatile uint32_t last_ack_irq_entry_cc;
  volatile uint32_t last_ack_core_entry_cc;
  volatile uint32_t last_ack_before_txreq_cc;
  volatile uint32_t last_ack_delay_us;
  volatile uint32_t last_ack_ramp_up_cc;
  volatile uint32_t last_ack_now_cc;
  volatile uint32_t last_ack_fem_cc;
  volatile uint32_t last_ack_arm_result;
  volatile uint32_t last_bad_ack_reason;
  volatile uint32_t last_bad_ack_parse_ok;
  volatile uint32_t last_bad_ack_phr;
  volatile uint32_t last_bad_ack_fcf0;
  volatile uint32_t last_bad_ack_fcf1;
  volatile uint32_t last_bad_ack_dsn;
  volatile uint32_t last_bad_ack_tx_dsn;
} nrf54l15_radio_debug_t;

extern nrf54l15_radio_debug_t nrf54l15_radio_debug;

void nrf54l15_radio_debug_reset(void);

#endif /* NRF54L15_RADIO_DEBUG_H_ */

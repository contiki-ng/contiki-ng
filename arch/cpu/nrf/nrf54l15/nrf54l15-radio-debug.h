/*
 * Copyright (c) 2026, Contiki-NG.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NRF54L15_RADIO_DEBUG_H_
#define NRF54L15_RADIO_DEBUG_H_

#include <stdint.h>

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
  volatile uint32_t ack_delay_too_short;
  volatile uint32_t last_ack_irq_entry_cc;
  volatile uint32_t last_ack_core_entry_cc;
  volatile uint32_t last_ack_before_txreq_cc;
  volatile uint32_t last_ack_delay_us;
  volatile uint32_t last_ack_ramp_up_cc;
  volatile uint32_t last_ack_now_cc;
  volatile uint32_t last_ack_fem_cc;
  volatile uint32_t last_ack_arm_result;
} nrf54l15_radio_debug_t;

extern nrf54l15_radio_debug_t nrf54l15_radio_debug;

void nrf54l15_radio_debug_reset(void);

#endif /* NRF54L15_RADIO_DEBUG_H_ */

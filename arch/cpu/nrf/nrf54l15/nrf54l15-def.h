/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * CPU capabilities description for the nRF54L15 application core.
 * Keep the structure aligned with existing nRF ports.
 */
#ifndef NRF54L15_DEF_H_
#define NRF54L15_DEF_H_

#define NRF_HAS_UARTE   1
#define NRF_HAS_USB     0

/* nrf_802154 acknowledges received frames in hardware within the
 * turnaround time; a software ACK from CSMA would only follow late. */
#ifndef CSMA_CONF_SEND_SOFT_ACK
#define CSMA_CONF_SEND_SOFT_ACK 0
#endif

#endif /* NRF54L15_DEF_H_ */

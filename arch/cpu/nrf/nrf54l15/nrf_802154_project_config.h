/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * nrf_802154 library configuration for the Contiki-NG nRF54L15 port.
 */

#ifndef NRF_802154_PROJECT_CONFIG_H_
#define NRF_802154_PROJECT_CONFIG_H_

/* On nRF54LX the driver owns the RADIO IRQ directly. */
#define NRF_802154_INTERNAL_RADIO_IRQ_HANDLING 1
#define NRF_802154_INTERNAL_SWI_IRQ_HANDLING   0

/* Use direct (non-SWI) notification/request to avoid EGU dependency. */
#define NRF_802154_NOTIFICATION_IMPL NRF_802154_NOTIFICATION_IMPL_DIRECT
#define NRF_802154_REQUEST_IMPL      NRF_802154_REQUEST_IMPL_DIRECT

/* CCA defaults (ED mode, threshold/correlation values) used by nrf_802154. */
#define NRF_802154_CCA_MODE_DEFAULT            NRF_RADIO_CCA_MODE_ED
#define NRF_802154_CCA_ED_THRESHOLD_DBM_DEFAULT (-75)
#define NRF_802154_CCA_CORR_THRESHOLD_DEFAULT  45
#define NRF_802154_CCA_CORR_LIMIT_DEFAULT      2
#define NRF_802154_PENDING_SHORT_ADDRESSES     16
#define NRF_802154_PENDING_EXTENDED_ADDRESSES  16
#define NRF_802154_RX_BUFFERS                  20

/* Match Nordic's SL-opensource integration: frame timestamping is disabled. */
#define NRF_802154_FRAME_TIMESTAMP_ENABLED     0

/* nRF54L15 on the application core needs extra ACK turnaround slack on this
 * bare-metal Contiki port; the timer backend reduces but does not eliminate
 * the remaining scheduling gap. */
#define NRF_802154_ACK_IFS_EXTRA_TIME_US       160U

/* Disable features not available in the open-source SL or not needed. */
#define NRF_802154_ENCRYPTION_ENABLED          0
#define NRF_802154_IE_WRITER_ENABLED           0
#define NRF_802154_SECURITY_WRITER_ENABLED     0
#define NRF_802154_CSMA_CA_ENABLED             0
#define NRF_802154_DELAYED_TRX_ENABLED         0
#define NRF_802154_ACK_TIMEOUT_ENABLED         1
#define NRF_802154_IFS_ENABLED                 0
#define NRF_802154_CARRIER_FUNCTIONS_ENABLED   0
#define NRF_802154_TEST_MODES_ENABLED          0

/* Do not report CRC errors to keep things simple. */
#define NRF_802154_NOTIFY_CRCERROR             0

/* We do not use the internal serialization. */
#define NRF_802154_SERIALIZATION_HOST          0

/* Do not use internal includes (no closed-source SL).
 * The library checks #ifdef, so we must NOT define this at all. */
#ifdef NRF_802154_USE_INTERNAL_INCLUDES
#undef NRF_802154_USE_INTERNAL_INCLUDES
#endif

#endif /* NRF_802154_PROJECT_CONFIG_H_ */

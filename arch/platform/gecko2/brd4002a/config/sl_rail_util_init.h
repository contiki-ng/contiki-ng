/***************************************************************************//**
 * @file
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#ifndef SL_RAIL_UTIL_INIT_H
#define SL_RAIL_UTIL_INIT_H

#include "rail.h"
#include "sl_rail_util_init_inst0_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum sl_rail_util_handle_type{
  SL_RAIL_UTIL_HANDLE_INST0,
} sl_rail_util_handle_type_t;

/**
 * Initialize the RAIL Init Utility.
 *
 * @note: This function should be called during application initialization.
 */
void sl_rail_util_init(void);

/**
 * Get the RAIL handle created during initialization.
 * @param[in] handle The enum name of the desired RAIL handle.
 *
 * @return A valid RAIL handle. If the RAIL handle hasn't been set up, the
 * invalid value of \ref RAIL_EFR32_HANDLE will be returned.
 */
RAIL_Handle_t sl_rail_util_get_handle(sl_rail_util_handle_type_t handle);

/**
 * A callback available to the application, called on RAIL asserts.
 *
 * @param[in] rail_handle The RAIL handle associated with the assert.
 * @param[in] error_code The assertion error code.
 */
void sl_rail_util_on_assert_failed(RAIL_Handle_t rail_handle,
                                   RAIL_AssertErrorCodes_t error_code);

/**
 * A callback available to the application, called on RAIL init completion.
 *
 * @param[in] rail_handle The RAIL handle associated with the RAIL init
 * completion notification.
 */
void sl_rail_util_on_rf_ready(RAIL_Handle_t rail_handle);

/**
 * A callback available to the application, called on a channel configuration
 * change.
 *
 * @param[in] rail_handle The RAIL handle associated with the channel config
 * change notification.
 * @param[in] entry The channel configuration being changed to.
 */
void sl_rail_util_on_channel_config_change(RAIL_Handle_t rail_handle,
                                           const RAIL_ChannelConfigEntry_t *entry);

/**
 * A callback available to the application, called on registered RAIL events.
 *
 * @param[in] rail_handle The RAIL handle associated with the RAIL event
 * notification.
 * @param[in] events The RAIL events having occurred.
 */
void sl_rail_util_on_event(RAIL_Handle_t rail_handle,
                           RAIL_Events_t events);

/**
 * An event mask, available to the application, specifying the radio events
 * setup within the init code.
 *
 * @note: Because the value of this define is evaluated based on values in the
 * \ref RAIL_Events_t enum, this define will only have a valid value during
 * run-time.
 */
#define SL_RAIL_UTIL_INIT_EVENT_INST0_MASK (RAIL_EVENTS_NONE              \
  | (SL_RAIL_UTIL_INIT_EVENT_RSSI_AVERAGE_DONE_INST0_ENABLE               \
    ? RAIL_EVENT_RSSI_AVERAGE_DONE : RAIL_EVENTS_NONE)                                 \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_ACK_TIMEOUT_INST0_ENABLE                  \
    ? RAIL_EVENT_RX_ACK_TIMEOUT : RAIL_EVENTS_NONE)                                    \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_FIFO_ALMOST_FULL_INST0_ENABLE             \
    ? RAIL_EVENT_RX_FIFO_ALMOST_FULL : RAIL_EVENTS_NONE)                               \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_PACKET_RECEIVED_INST0_ENABLE              \
    ? RAIL_EVENT_RX_PACKET_RECEIVED : RAIL_EVENTS_NONE)                                \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_PREAMBLE_LOST_INST0_ENABLE                \
    ? RAIL_EVENT_RX_PREAMBLE_LOST : RAIL_EVENTS_NONE)                                  \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_PREAMBLE_DETECT_INST0_ENABLE              \
    ? RAIL_EVENT_RX_PREAMBLE_DETECT : RAIL_EVENTS_NONE)                                \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_SYNC1_DETECT_INST0_ENABLE                 \
    ? RAIL_EVENT_RX_SYNC1_DETECT : RAIL_EVENTS_NONE)                                   \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_SYNC2_DETECT_INST0_ENABLE                 \
    ? RAIL_EVENT_RX_SYNC2_DETECT : RAIL_EVENTS_NONE)                                   \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_FRAME_ERROR_INST0_ENABLE                  \
    ? RAIL_EVENT_RX_FRAME_ERROR : RAIL_EVENTS_NONE)                                    \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_FIFO_FULL_INST0_ENABLE                    \
    ? RAIL_EVENT_RX_FIFO_FULL : RAIL_EVENTS_NONE)                                      \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_FIFO_OVERFLOW_INST0_ENABLE                \
    ? RAIL_EVENT_RX_FIFO_OVERFLOW : RAIL_EVENTS_NONE)                                  \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_ADDRESS_FILTERED_INST0_ENABLE             \
    ? RAIL_EVENT_RX_ADDRESS_FILTERED : RAIL_EVENTS_NONE)                               \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_TIMEOUT_INST0_ENABLE                      \
    ? RAIL_EVENT_RX_TIMEOUT : RAIL_EVENTS_NONE)                                        \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_TX_SCHEDULED_RX_TX_STARTED_INST0_ENABLE   \
    ? RAIL_EVENT_SCHEDULED_RX_STARTED : RAIL_EVENTS_NONE)                              \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_SCHEDULED_RX_END_INST0_ENABLE             \
    ? RAIL_EVENT_RX_SCHEDULED_RX_END : RAIL_EVENTS_NONE)                               \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_SCHEDULED_RX_MISSED_INST0_ENABLE          \
    ? RAIL_EVENT_RX_SCHEDULED_RX_MISSED : RAIL_EVENTS_NONE)                            \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_PACKET_ABORTED_INST0_ENABLE               \
    ? RAIL_EVENT_RX_PACKET_ABORTED : RAIL_EVENTS_NONE)                                 \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_FILTER_PASSED_INST0_ENABLE                \
    ? RAIL_EVENT_RX_FILTER_PASSED : RAIL_EVENTS_NONE)                                  \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_TIMING_LOST_INST0_ENABLE                  \
    ? RAIL_EVENT_RX_TIMING_LOST : RAIL_EVENTS_NONE)                                    \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_TIMING_DETECT_INST0_ENABLE                \
    ? RAIL_EVENT_RX_TIMING_DETECT : RAIL_EVENTS_NONE)                                  \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_CHANNEL_HOPPING_COMPLETE_INST0_ENABLE     \
    ? RAIL_EVENT_RX_CHANNEL_HOPPING_COMPLETE : RAIL_EVENTS_NONE)                       \
  | (SL_RAIL_UTIL_INIT_EVENT_RX_DUTY_CYCLE_RX_END_INST0_ENABLE            \
    ? RAIL_EVENT_RX_DUTY_CYCLE_RX_END : RAIL_EVENTS_NONE)                              \
  | (SL_RAIL_UTIL_INIT_EVENT_IEEE802154_DATA_REQUEST_COMMAND_INST0_ENABLE \
    ? RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND : RAIL_EVENTS_NONE)                   \
  | (SL_RAIL_UTIL_INIT_EVENT_ZWAVE_BEAM_INST0_ENABLE                      \
    ? RAIL_EVENT_ZWAVE_BEAM : RAIL_EVENTS_NONE)                                        \
  | (SL_RAIL_UTIL_INIT_EVENT_ZWAVE_LR_ACK_REQUEST_COMMAND_INST0_ENABLE    \
    ? RAIL_EVENT_ZWAVE_LR_ACK_REQUEST_COMMAND : RAIL_EVENTS_NONE)                      \
  | (SL_RAIL_UTIL_INIT_EVENT_TX_FIFO_ALMOST_EMPTY_INST0_ENABLE            \
    ? RAIL_EVENT_TX_FIFO_ALMOST_EMPTY : RAIL_EVENTS_NONE)                              \
  | (SL_RAIL_UTIL_INIT_EVENT_TX_PACKET_SENT_INST0_ENABLE                  \
    ? RAIL_EVENT_TX_PACKET_SENT : RAIL_EVENTS_NONE)                                    \
  | (SL_RAIL_UTIL_INIT_EVENT_TXACK_PACKET_SENT_INST0_ENABLE               \
    ? RAIL_EVENT_TXACK_PACKET_SENT : RAIL_EVENTS_NONE)                                 \
  | (SL_RAIL_UTIL_INIT_EVENT_TX_ABORTED_INST0_ENABLE                      \
    ? RAIL_EVENT_TX_ABORTED : RAIL_EVENTS_NONE)                                        \
  | (SL_RAIL_UTIL_INIT_EVENT_TXACK_ABORTED_INST0_ENABLE                   \
    ? RAIL_EVENT_TXACK_ABORTED : RAIL_EVENTS_NONE)                                     \
  | (SL_RAIL_UTIL_INIT_EVENT_TX_BLOCKED_INST0_ENABLE                      \
    ? RAIL_EVENT_TX_BLOCKED : RAIL_EVENTS_NONE)                                        \
  | (SL_RAIL_UTIL_INIT_EVENT_TXACK_BLOCKED_INST0_ENABLE                   \
    ? RAIL_EVENT_TXACK_BLOCKED : RAIL_EVENTS_NONE)                                     \
  | (SL_RAIL_UTIL_INIT_EVENT_TX_UNDERFLOW_INST0_ENABLE                    \
    ? RAIL_EVENT_TX_UNDERFLOW : RAIL_EVENTS_NONE)                                      \
  | (SL_RAIL_UTIL_INIT_EVENT_TXACK_UNDERFLOW_INST0_ENABLE                 \
    ? RAIL_EVENT_TXACK_UNDERFLOW : RAIL_EVENTS_NONE)                                   \
  | (SL_RAIL_UTIL_INIT_EVENT_TX_CHANNEL_CLEAR_INST0_ENABLE                \
    ? RAIL_EVENT_TX_CHANNEL_CLEAR  : RAIL_EVENTS_NONE)                                 \
  | (SL_RAIL_UTIL_INIT_EVENT_TX_CHANNEL_BUSY_INST0_ENABLE                 \
    ? RAIL_EVENT_TX_CHANNEL_BUSY : RAIL_EVENTS_NONE)                                   \
  | (SL_RAIL_UTIL_INIT_EVENT_TX_CCA_RETRY_INST0_ENABLE                    \
    ? RAIL_EVENT_TX_CCA_RETRY : RAIL_EVENTS_NONE)                                      \
  | (SL_RAIL_UTIL_INIT_EVENT_TX_START_CCA_INST0_ENABLE                    \
    ? RAIL_EVENT_TX_START_CCA : RAIL_EVENTS_NONE)                                      \
  | (SL_RAIL_UTIL_INIT_EVENT_TX_STARTED_INST0_ENABLE                      \
    ? RAIL_EVENT_TX_STARTED : RAIL_EVENTS_NONE)                                        \
  | (SL_RAIL_UTIL_INIT_EVENT_TX_SCHEDULED_TX_MISSED_INST0_ENABLE          \
    ? RAIL_EVENT_TX_SCHEDULED_TX_MISSED : RAIL_EVENTS_NONE)                            \
  | (SL_RAIL_UTIL_INIT_EVENT_CONFIG_UNSCHEDULED_INST0_ENABLE              \
    ? RAIL_EVENT_CONFIG_UNSCHEDULED : RAIL_EVENTS_NONE)                                \
  | (SL_RAIL_UTIL_INIT_EVENT_CONFIG_SCHEDULED_INST0_ENABLE                \
    ? RAIL_EVENT_CONFIG_SCHEDULED : RAIL_EVENTS_NONE)                                  \
  | (SL_RAIL_UTIL_INIT_EVENT_SCHEDULER_STATUS_INST0_ENABLE                \
    ? RAIL_EVENT_SCHEDULER_STATUS : RAIL_EVENTS_NONE)                                  \
  | (SL_RAIL_UTIL_INIT_EVENT_CAL_NEEDED_INST0_ENABLE                      \
    ? RAIL_EVENT_CAL_NEEDED : RAIL_EVENTS_NONE)                                        \
  | (SL_RAIL_UTIL_INIT_EVENT_DETECT_RSSI_THRESHOLD_INST0_ENABLE           \
    ? RAIL_EVENT_DETECT_RSSI_THRESHOLD : RAIL_EVENTS_NONE)                             \
  | (SL_RAIL_UTIL_INIT_EVENT_THERMISTOR_DONE_INST0_ENABLE                 \
    ? RAIL_EVENT_THERMISTOR_DONE : RAIL_EVENTS_NONE)                                   \
  | (SL_RAIL_UTIL_INIT_EVENT_TX_BLOCKED_TOO_HOT_INST0_ENABLE              \
    ? RAIL_EVENT_TX_BLOCKED_TOO_HOT : RAIL_EVENTS_NONE)                                \
  | (SL_RAIL_UTIL_INIT_EVENT_TEMPERATURE_TOO_HOT_INST0_ENABLE             \
    ? RAIL_EVENT_TEMPERATURE_TOO_HOT : RAIL_EVENTS_NONE)                               \
  | (SL_RAIL_UTIL_INIT_EVENT_TEMPERATURE_COOL_DOWN_INST0_ENABLE           \
    ? RAIL_EVENT_TEMPERATURE_COOL_DOWN : RAIL_EVENTS_NONE))

/**
 * An inverted event mask, available to the application, specifying the radio
 * events setup within the init code.
 *
 * @note: Because the value of this define is evaluated based on values in the
 * \ref RAIL_Events_t enum, this define will only have a valid value during
 * run-time.
 */
#define SL_RAIL_UTIL_INIT_EVENT_INST0_INVERSE_MASK \
  (~SL_RAIL_UTIL_INIT_EVENT_INST0_MASK)

#ifdef __cplusplus
}
#endif

#endif // SL_RAIL_UTIL_INIT_H

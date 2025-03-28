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

#ifndef SL_RAIL_UTIL_PROTOCOL_CONFIG_H
#define SL_RAIL_UTIL_PROTOCOL_CONFIG_H

#include "sl_rail_util_protocol_types.h"

// <<< Use Configuration Wizard in Context Menu >>>

// <h> Bluetooth LE Settings
// <h> BLE: Transition Times
// <q SL_RAIL_UTIL_PROTOCOL_BLE_ENABLE> Enable/Disable BLE
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_BLE_ENABLE 1
// <o SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_IDLE_TO_RX_US> Transition time (microseconds) from idle to RX
// <0-65535:1>
// <i> Default: 100
#define SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_IDLE_TO_RX_US  100
// <o SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_TX_TO_RX_US> Transition time (microseconds) from TX to RX
// <0-65535:1>
// <i> Default: 150
#define SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_TX_TO_RX_US  150
// <o SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_IDLE_TO_TX_US> Transition time (microseconds) from idle to TX
// <0-65535:1>
// <i> Default: 100
#define SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_IDLE_TO_TX_US  100
// <o SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_RX_TO_TX_US> Transition time (microseconds) from RX to TX
// <0-65535:1>
// <i> Default: 150
#define SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_RX_TO_TX_US  150
// </h>

// <h> BLE: RX Search Timeouts
// <e SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_ENABLE> Enable RX Search timeout after Idle
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_ENABLE 0
// <o SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_US> Max time (microseconds) radio will search for packet when coming from idle
// <1-65535:1>
// <i> Default: 65535
#define SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_US  65535
// </e>
// <e SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_ENABLE> Enable RX Search timeout after TX
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_ENABLE 0
// <o SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_US> Max time (microseconds) radio will search for packet when coming from TX
// <1-65535:1>
// <i> Default: 65535
#define SL_RAIL_UTIL_PROTOCOL_BLE_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_US  65535
// </e>
// </h>
// </h>
// <h> IEEE 802.15.4, GB868 Settings
// <h> GB868: Node Configuration
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_ENABLE> Enable/Disable IEEE 802.15.4 868 Hz Protocol
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_ENABLE 1
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_PAN_COORDINATOR_ENABLE> PAN Coordinator
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_PAN_COORDINATOR_ENABLE 0
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_PROMISCUOUS_MODE_ENABLE> Promiscuous Mode
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_PROMISCUOUS_MODE_ENABLE 1
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_DEFAULT_FRAME_PENDING_STATE> Default Frame Pending bit value for outgoing ACKs in response to Data Request Command
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_DEFAULT_FRAME_PENDING_STATE 0
// </h>

// <h> GB868: Receivable Frame Types
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_ACCEPT_BEACON_FRAME_ENABLE> Beacon Frames
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_ACCEPT_BEACON_FRAME_ENABLE 1
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_ACCEPT_DATA_FRAME_ENABLE> Data Frames
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_ACCEPT_DATA_FRAME_ENABLE 1
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_ACCEPT_ACK_FRAME_ENABLE> ACK Frames
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_ACCEPT_ACK_FRAME_ENABLE 0
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_ACCEPT_COMMAND_FRAME_ENABLE> Command Frames
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_ACCEPT_COMMAND_FRAME_ENABLE 1
// </h>

// <h> GB868: Transition Times
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_IDLE_TO_RX_US> Transition time (microseconds) from idle to RX
// <0-65535:1>
// <i> Default: 100
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_IDLE_TO_RX_US  100
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_TX_TO_RX_US> Transition time (microseconds) from TX to RX
// <0-65535:1>
// <i> Default: 440
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_TX_TO_RX_US  440
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_IDLE_TO_TX_US> Transition time (microseconds) from idle to TX
// <0-65535:1>
// <i> Default: 100
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_IDLE_TO_TX_US  100
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_RX_TO_TX_US> Transition time (microseconds) from RX to TX
// <0-65535:1>
// <i> Default: 450
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_RX_TO_TX_US  450
// </h>

// <h> GB868: RX Search Timeouts
// <e SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_ENABLE> Enable RX Search timeout after Idle
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_ENABLE 0
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_US> Max time (microseconds) radio will search for packet when coming from idle
// <1-65535:1>
// <i> Default: 65535
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_US  65535
// </e>
// <e SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_ENABLE> Enable RX Search timeout after TX
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_ENABLE 0
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_US> Max time (microseconds) radio will search for packet when coming from TX
// <1-65535:1>
// <i> Default: 65535
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_US  65535
// </e>
// </h>

// <h> GB868: Auto ACK Configuration
// <e SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_AUTO_ACK_ENABLE> Enable Auto ACKs
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_AUTO_ACK_ENABLE 1
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_AUTO_ACK_TIMEOUT_US> RX ACK timeout duration (microseconds)
// <1-65535:1>
// <i> Default: 1800
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_AUTO_ACK_TIMEOUT_US  1800
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_AUTO_ACK_RX_TRANSITION_STATE> Radio state transition after attempting to receive ACK
// <RAIL_RF_STATE_IDLE=> Idle
// <RAIL_RF_STATE_RX=> RX
// <i> Default: RAIL_RF_STATE_RX
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_AUTO_ACK_RX_TRANSITION_STATE  RAIL_RF_STATE_RX
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_AUTO_ACK_TX_TRANSITION_STATE> Radio state transition after transmitting ACK
// <RAIL_RF_STATE_IDLE=> Idle
// <RAIL_RF_STATE_RX=> RX
// <i> Default: RAIL_RF_STATE_RX
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_AUTO_ACK_TX_TRANSITION_STATE  RAIL_RF_STATE_RX
// </e>
// </h>
// </h>

// <h> Z-Wave Settings
// <h> Z-Wave: Configuration Options
// <q SL_RAIL_UTIL_PROTOCOL_ZWAVE_ENABLE> Enable/Disable Z-Wave
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_ENABLE 1
// <q SL_RAIL_UTIL_PROTOCOL_ZWAVE_PROMISCUOUS_MODE_ENABLE> Enable Promiscuous Mode
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_PROMISCUOUS_MODE_ENABLE 1
// <q SL_RAIL_UTIL_PROTOCOL_ZWAVE_DETECT_BEAM_FRAME_ENABLE> Accept Beam Frames
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_DETECT_BEAM_FRAME_ENABLE 1
// <q SL_RAIL_UTIL_PROTOCOL_ZWAVE_NODE_ID_FILTERING_ENABLE> Filter Packets Based on Node ID
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_NODE_ID_FILTERING_ENABLE 0
// <q SL_RAIL_UTIL_PROTOCOL_ZWAVE_PROMISCUOUS_BEAM_MODE_ENABLE> Enable Promiscuous Beam Mode
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_PROMISCUOUS_BEAM_MODE_ENABLE 0
// </h>

// <h> Z-Wave: Transition Times
// <o SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_IDLE_TO_RX_US> Transition time (microseconds) from idle to RX
// <0-65535:1>
// <i> Default: 100
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_IDLE_TO_RX_US  100
// <o SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_TX_TO_RX_US> Transition time (microseconds) from TX to RX
// <0-65535:1>
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_TX_TO_RX_US  0
// <o SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_IDLE_TO_TX_US> Transition time (microseconds) from idle to TX
// <0-65535:1>
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_IDLE_TO_TX_US  0
// <o SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_RX_TO_TX_US> Transition time (microseconds) from RX to TX
// <0-65535:1>
// <i> Default: 1000
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_RX_TO_TX_US  1000
// </h>

// <h> Z-Wave: RX Search Timeouts
// <e SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_ENABLE> Enable RX Search timeout after Idle
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_ENABLE 0
// <o SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_US> Max time (microseconds) radio will search for packet when coming from idle
// <1-65535:1>
// <i> Default: 65535
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_US  65535
// </e>
// <e SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_ENABLE> Enable RX Search timeout after TX
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_ENABLE 0
// <o SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_US> Max time (microseconds) radio will search for packet when coming from TX
// <1-65535:1>
// <i> Default: 65535
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_US  65535
// </e>
// </h>

// <h> Z-Wave: Auto ACK Configuration
// <e SL_RAIL_UTIL_PROTOCOL_ZWAVE_AUTO_ACK_ENABLE> Enable Auto ACKs
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_AUTO_ACK_ENABLE 0
// <o SL_RAIL_UTIL_PROTOCOL_ZWAVE_AUTO_ACK_TIMEOUT_US> RX ACK timeout duration (microseconds)
// <1-9600:1>
// <i> Default: 9600
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_AUTO_ACK_TIMEOUT_US  9600
// <o SL_RAIL_UTIL_PROTOCOL_ZWAVE_AUTO_ACK_RX_TRANSITION_STATE> Radio state transition after attempting to receive ACK
// <RAIL_RF_STATE_IDLE=> Idle
// <RAIL_RF_STATE_RX=> RX
// <i> Default: RAIL_RF_STATE_RX
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_AUTO_ACK_RX_TRANSITION_STATE  RAIL_RF_STATE_RX
// <o SL_RAIL_UTIL_PROTOCOL_ZWAVE_AUTO_ACK_TX_TRANSITION_STATE> Radio state transition after transmitting ACK
// <RAIL_RF_STATE_IDLE=> Idle
// <RAIL_RF_STATE_RX=> RX
// <i> Default: RAIL_RF_STATE_RX
#define SL_RAIL_UTIL_PROTOCOL_ZWAVE_AUTO_ACK_TX_TRANSITION_STATE  RAIL_RF_STATE_RX
// </e>
// </h>
// </h>

// <h> Sidewalk Settings
// <h> Sidewalk: Configuration Options
// <q SL_RAIL_UTIL_PROTOCOL_SIDEWALK_ENABLE> Enable/Disable Sidewalk
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_SIDEWALK_ENABLE 0
// </h>
// </h>

// <h> IEEE 802.15.4, 2.4 GHz Settings
// <h> 2.4 GHz: Node Configuration
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_ENABLE> Enable/Disable IEEE 802.15.4 2.4 GHz Protocol
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_ENABLE 1
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_PAN_COORDINATOR_ENABLE> PAN Coordinator
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_PAN_COORDINATOR_ENABLE 0
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_PROMISCUOUS_MODE_ENABLE> Promiscuous Mode
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_PROMISCUOUS_MODE_ENABLE 1
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_DEFAULT_FRAME_PENDING_STATE> Default Frame Pending bit value for outgoing ACKs in response to Data Request Command
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_DEFAULT_FRAME_PENDING_STATE 0
// </h>

// <h> 2.4 GHz: Receivable Frame Types
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_ACCEPT_BEACON_FRAME_ENABLE> Beacon Frames
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_ACCEPT_BEACON_FRAME_ENABLE 1
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_ACCEPT_DATA_FRAME_ENABLE> Data Frames
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_ACCEPT_DATA_FRAME_ENABLE 1
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_ACCEPT_ACK_FRAME_ENABLE> ACK Frames
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_ACCEPT_ACK_FRAME_ENABLE 0
// <q SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_ACCEPT_COMMAND_FRAME_ENABLE> Command Frames
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_ACCEPT_COMMAND_FRAME_ENABLE 1
// </h>

// <h> 2.4 GHz: Transition Times
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_IDLE_TO_RX_US> Transition time (microseconds) from idle to RX
// <0-65535:1>
// <i> Default: 100
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_IDLE_TO_RX_US  100
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_TX_TO_RX_US> Transition time (microseconds) from TX to RX
// <0-65535:1>
// <i> Default: 182
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_TX_TO_RX_US  182
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_IDLE_TO_TX_US> Transition time (microseconds) from idle to TX
// <0-65535:1>
// <i> Default: 100
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_IDLE_TO_TX_US  100
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_RX_TO_TX_US> Transition time (microseconds) from RX to TX
// <0-65535:1>
// <i> Default: 192
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_RX_TO_TX_US  192
// </h>

// <h> 2.4 GHz: RX Search Timeouts
// <e SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_ENABLE> Enable RX Search timeout after Idle
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_ENABLE 0
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_US> Max time (microseconds) radio will search for packet when coming from idle
// <1-65535:1>
// <i> Default: 65535
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_RX_SEARCH_TIMEOUT_AFTER_IDLE_US  65535
// </e>
// <e SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_ENABLE> Enable RX Search timeout after TX
// <i> Default: 0
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_ENABLE 0
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_US> Max time (microseconds) radio will search for packet when coming from TX
// <1-65535:1>
// <i> Default: 65535
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_TIMING_RX_SEARCH_TIMEOUT_AFTER_TX_US  65535
// </e>
// </h>

// <h> 2.4 GHz: Auto ACK Configuration
// <e SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_AUTO_ACK_ENABLE> Enable Auto ACKs
// <i> Default: 1
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_AUTO_ACK_ENABLE 1
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_AUTO_ACK_TIMEOUT_US> RX ACK timeout duration (microseconds)
// <1-65535:1>
// <i> Default: 672
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_AUTO_ACK_TIMEOUT_US  672
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_AUTO_ACK_RX_TRANSITION_STATE> Radio state transition after attempting to receive ACK
// <RAIL_RF_STATE_IDLE=> Idle
// <RAIL_RF_STATE_RX=> RX
// <i> Default: RAIL_RF_STATE_RX
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_AUTO_ACK_RX_TRANSITION_STATE  RAIL_RF_STATE_RX
// <o SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_AUTO_ACK_TX_TRANSITION_STATE> Radio state transition after transmitting ACK
// <RAIL_RF_STATE_IDLE=> Idle
// <RAIL_RF_STATE_RX=> RX
// <i> Default: RAIL_RF_STATE_RX
#define SL_RAIL_UTIL_PROTOCOL_IEEE802154_2P4GHZ_AUTO_ACK_TX_TRANSITION_STATE  RAIL_RF_STATE_RX
// </e>
// </h>
// </h>

// <<< end of configuration section >>>

#endif // SL_RAIL_UTIL_PROTOCOL_CONFIG_H

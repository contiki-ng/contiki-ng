/*
 * Copyright (c) 2005, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Header file for the radio API
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 */

/**
 * \addtogroup dev
 * @{
 */

/**
 * \defgroup radio Radio API
 *
 * The radio API module defines a set of functions that a radio device
 * driver must implement.
 *
 * @{
 */

#ifndef RADIO_H_
#define RADIO_H_

#include <stddef.h>

/**
 * Each radio has a set of parameters that designate the current
 * configuration and state of the radio. Parameters can either have
 * values of type `radio_value_t`, or, when this type is insufficient, a
 * generic object that is specified by a memory pointer and the size
 * of the object.
 *
 * The `radio_value_t` type is set to an integer type that can hold most
 * values used to configure the radio, and is therefore the most
 * common type used for a parameter. Certain parameters require
 * objects of a considerably larger size than `radio_value_t`, however,
 * and in these cases the documentation below for the parameter will
 * indicate this.
 *
 * All radio parameters that can vary during runtime are prefixed by
 * `RADIO_PARAM_`, whereas those "parameters" that are guaranteed to
 * remain immutable are prefixed by `RADIO_CONST_`. Each mutable
 * parameter has a set of valid parameter values. When attempting to
 * set a parameter to an invalid value, the radio will return
 * `RADIO_RESULT_INVALID_VALUE`.
 *
 * Some radios support only a subset of the defined radio parameters.
 * When trying to set or get such an unsupported parameter, the radio
 * will return `RADIO_RESULT_NOT_SUPPORTED`.
 */

typedef int radio_value_t;
typedef unsigned radio_param_t;

/**
 * \brief Radio parameters and constants
 *
 * The fields of this enum are expected to be used as the `param` argument
 * of `get_value()`, `set_value()`, `get_object()` and `set_object()`.
 *
 * More specifically, fields prefixed with `RADIO_PARAM_` may be passed as an
 * argument to any of those four functions. Exceptions are documented on a
 * per-field basis. Fields prefixed with `RADIO_CONST_` will only be passed as
 * an argument to `get_value()` and `get_object()`.
 */
enum radio_param_e {

  /**
   * When getting the value of this parameter, the radio driver should
   * indicate whether the radio is on or not.
   *
   * `RADIO_POWER_MODE_ON`: The radio is powered and ready to receive frames
   * `RADIO_POWER_MODE_OFF`: The radio is powered off
   *
   * When setting the value of this parameter, the driver should put the radio
   * part in the corresponding state.
   * `RADIO_POWER_MODE_ON`: The radio should be powered on and ready to receive
   * frames. This is equivalent to a call to `NETSTACK_RADIO.on()`.
   * `RADIO_POWER_MODE_OFF`: The radio should be put in the lowest power
   * consumption state available. This is equivalent to a call to
   * `NETSTACK_RADIO.off()`.
   */
  RADIO_PARAM_POWER_MODE,

  /**
   * Channel used for radio communication. The channel depends on the
   * communication standard used by the radio. The values can range
   * from `RADIO_CONST_CHANNEL_MIN` to `RADIO_CONST_CHANNEL_MAX`.
   *
   * When setting this parameter, the change should take effect immediately
   * if the radio is in `RADIO_POWER_MODE_ON`. Otherwise the change should take
   * effect the next time the radio turns on.
   *
   * When reading this parameter, the driver should return the currently
   * configured channel if the radio is in `RADIO_POWER_MODE_ON`, or the last
   * used channel is the radio is currently in `RADIO_POWER_MODE_OFF`.
   */
  RADIO_PARAM_CHANNEL,

  /**
   * The personal area network identifier (PAN ID), which is used by the h/w
   * frame filtering functionality of some radios.
   *
   * Setting this param will typically require the radio driver to commit the
   * PAN ID to some radio hardware register used for frame filtering.
   *
   * Getting this param will typically require the radio driver to return the
   * value currently stored in the respective hardware register.
   *
   * If the hardware does not support frame filtering, there is no expectation
   * to perform such filtering in the radio driver software. In the case of
   * such radios, the driver can simply return `RADIO_RESULT_NOT_SUPPORTED`.
   */
  RADIO_PARAM_PAN_ID,

  /**
   * The short address (16 bits) for the radio, which is used by the h/w
   * filter.
   *
   * Setting this param will typically require the radio driver to commit the
   * value to some radio hardware register used for frame filtering.
   *
   * Getting this param will typically require the radio driver to return the
   * value currently stored in the respective hardware register.
   *
   * If the hardware does not support frame filtering, there is no expectation
   * to perform such filtering in the radio driver software. In the case of
   * such radios, the driver can simply return `RADIO_RESULT_NOT_SUPPORTED`.
   */
  RADIO_PARAM_16BIT_ADDR,

  /**
   * Radio receiver mode determines if the radio has address filter
   * (`RADIO_RX_MODE_ADDRESS_FILTER`) and auto-ACK (`RADIO_RX_MODE_AUTOACK`)
   * enabled. This parameter is set as a bit mask.
   */
  RADIO_PARAM_RX_MODE,

  /**
   * Radio transmission mode determines if the radio has send on CCA
   * (`RADIO_TX_MODE_SEND_ON_CCA`) enabled or not. This parameter is set
   * as a bit mask.
   */
  RADIO_PARAM_TX_MODE,

  /**
   * Transmission power in dBm. The values can range from
   * `RADIO_CONST_TXPOWER_MIN` to `RADIO_CONST_TXPOWER_MAX`.
   *
   * Some radios restrict the available values to a subset of this
   * range.  If an unavailable TXPOWER value is requested to be set,
   * the radio may select another TXPOWER close to the requested
   * one. When getting the value of this parameter, the actual value
   * used by the radio will be returned.
   */
  RADIO_PARAM_TXPOWER,

  /**
   * Clear channel assessment threshold in dBm. This threshold
   * determines the minimum RSSI level at which the radio will assume
   * that there is a packet in the air.
   *
   * The CCA threshold must be set to a level above the noise floor of
   * the deployment. Otherwise mechanisms such as send-on-CCA and
   * low-power-listening duty cycling protocols may not work
   * correctly. Hence, the default value of the system may not be
   * optimal for any given deployment.
   */
  RADIO_PARAM_CCA_THRESHOLD,

  /**
   * Received signal strength indicator in dBm.
   *
   * When getting this parameter, the radio driver should return the current
   * RSSI value as reported by the radio.
   *
   * This may require turning on the radio and requesting an RSSI sample.
   *
   * This parameter will only be passed as an argument to the `get_value()`
   * function.
   */
  RADIO_PARAM_RSSI,

  /**
   * The RSSI value of the last received packet.
   *
   * This parameter will only be passed as an argument to the `get_value()`
   * function.
   */
  RADIO_PARAM_LAST_RSSI,

  /**
    * The current I/Q LSBs.
    *
    * This parameter will only be passed as an argument to the `get_value()`
    * function.
    */
  RADIO_PARAM_IQ_LSBS,

  /**
   * Link quality indicator of the last received packet.
   *
   * The value returned should be an unsigned number between 0x00 and 0xFF.
   *
   * This parameter will only be passed as an argument to the `get_value()`
   * function.
   */
  RADIO_PARAM_LAST_LINK_QUALITY,

  /**
   * Long (64 bits) address for the radio, which is used by the address filter.
   * The address is specified in network byte order.
   *
   * Because this parameter value is larger than what fits in `radio_value_t`,
   * it needs to be used with `get_object()`/`set_object()`.
   *
   * Setting this param will typically require the radio driver to commit the
   * value to some radio hardware register used for frame filtering.
   *
   * Getting this param will typically require the radio driver to return the
   * value currently stored in the respective hardware register.
   *
   * If the hardware does not support frame filtering, there is no expectation
   * to perform such filtering in the radio driver software. In the case of
   * such radios, the driver can simply return `RADIO_RESULT_NOT_SUPPORTED`.
   */
  RADIO_PARAM_64BIT_ADDR,

  /**
   * Last packet timestamp, of type `rtimer_clock_t`.
   *
   * The timestamp corresponds to the point in time between the end of
   * reception of the synchronisation header and the start of reception of the
   * physical header (PHR).
   *
   * ```
   * +---------------+-----+---------------+---------------+-----+
   * |      SHR      | PHR |      MHR      |  MAC Payload  | MFR |
   * +---------------+-----+---------------+---------------+-----+
   *                 ^
   * --- Timestamp --|
   * ```
   *
   * Because this parameter value may be larger than what fits in `radio_value_t`,
   * it needs to be used with `get_object()`/`set_object()`.
   *
   * This parameter will only be passed as an argument to the `get_object()`
   * function.
   */
  RADIO_PARAM_LAST_PACKET_TIMESTAMP,

  /**
   * For enabling and disabling the SHR search
   *
   * Setting this param to `RADIO_SHR_SEARCH_DIS` will disable SHR search.
   * This means that when the radio is in receive mode it can be used to
   * sample RSSI or to perform a clear channel assessment (CCA), but it will
   * not receive frames.
   *
   * Setting this param to `RADIO_SHR_SEARCH_EN` will enable SHR search.
   * This means that when the radio is in receive mode it will receive frames
   * as normal.
   *
   * When setting this parameter, the change should take effect immediately
   * if the radio is in `RADIO_POWER_MODE_ON`. Otherwise the change should take
   * effect the next time the radio turns on.
   */
  RADIO_PARAM_SHR_SEARCH,

  /* Constants (read only) */

  /**
   * The lowest radio channel number
   */
  RADIO_CONST_CHANNEL_MIN,

  /**
   * The highest radio channel number
   */
  RADIO_CONST_CHANNEL_MAX,

  /**
   * The minimum transmission power in dBm
   */
  RADIO_CONST_TXPOWER_MIN,

  /**
   * The maximum transmission power in dBm.
   */
  RADIO_CONST_TXPOWER_MAX,

  /* A pointer to TSCH timings in micro-seconds (tsch_timeslot_timing_usec *) */
  RADIO_CONST_TSCH_TIMING,

  /**
   * The physical layer header (PHR) + MAC layer footer (MFR) overhead in
   * bytes. This does _not_ include the synchronisation header (SHR).
   *
   * For example, on IEEE 802.15.4 at 2.4 GHz this will be 3 bytes: 1 byte for
   * the frame length (PHR) + 2 bytes for the CRC (MFR)
   */
  RADIO_CONST_PHY_OVERHEAD,

  /**
   * The air time of one byte in usec, e.g. 32 for IEEE 802.15.4 at 2.4 GHz
   */
  RADIO_CONST_BYTE_AIR_TIME,

  /**
   * The delay in usec between a call to the radio API's transmit function and
   * the end of SFD transmission.
   */
  RADIO_CONST_DELAY_BEFORE_TX,

  /**
   * The delay in usec between turning on the radio and it being actually
   * listening (able to hear a preamble)
   */
  RADIO_CONST_DELAY_BEFORE_RX,

  /**
   * The delay in usec between the end of SFD reception for an incoming frame
   * and the radio API starting to return `receiving_packet() != 0`
   */
  RADIO_CONST_DELAY_BEFORE_DETECT,

  /*
   * The maximum payload the radio driver is able to handle.
   *
   * This includes the MAC header and MAC payload, but not any tail bytes
   * added automatically by the radio. For example, in the typical case of
   * .15.4 operation at 2.4GHz, this will be 125 bytes (127 bytes minus the
   * FCS / CRC16).
   *
   * This is the maximum number of bytes that:
   *   - The MAC layer will ask the radio driver to transmit.
   *     This corresponds to the payload_len argument of the prepare() and
   *     send() and the transmit_len argument of transmit().
   *   - The radio driver will deliver to the MAC layer after frame reception.
   *     The buf_len of the read function will typically be greater than or
   *     equal to this value.
   *
   * Supporting this constant in the radio driver's get_value function is
   * mandatory.
   */
  RADIO_CONST_MAX_PAYLOAD_LEN,
};

/**
 * Radio power modes
 *
 * Used as the `value` argument of `get_value()` / `set_value()` when `param`
 * is `RADIO_PARAM_POWER_MODE`.
 */
enum radio_power_mode_e {
  /**
   * Radio powered off and in the lowest possible power consumption state.
   */
  RADIO_POWER_MODE_OFF,

  /**
   * Radio powered on and able to receive frames.
   */
  RADIO_POWER_MODE_ON,

  /**
   * Radio powered on and emitting unmodulated carriers.
   */
  RADIO_POWER_MODE_CARRIER_ON,

  /**
   * Radio powered on, but not emitting unmodulated carriers.
   */
  RADIO_POWER_MODE_CARRIER_OFF
};

/**
 * Possible values of the `get_value()` / `set_value()` `value` argument when
 * the `param` argument is `RADIO_PARAM_SHR_SEARCH`.
 */
enum radio_shr_search_e {
  RADIO_SHR_SEARCH_DIS = 0, /**< Disable SHR search or SHR search is enabled */
  RADIO_SHR_SEARCH_EN = 1,  /**< Enable SHR search or SHR search is enabled */
};

/*---------------------------------------------------------------------------*/
/**
 * \name Radio RX mode
 *
 * The radio reception mode controls address filtering and automatic
 * transmission of acknowledgements in the radio (if such operations
 * are supported by the radio). A single parameter is used to allow
 * setting these features simultaneously as an atomic operation.
 *
 * These macros are meant to be used as the `value` argument of `get_value()`
 * and `set_value()` when the `param` argument is `RADIO_PARAM_RX_MODE`.
 *
 * To enable both address filter and transmissions of automatic
 * acknowledgments:
 *
 * ```
 * NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE,
 *                          RADIO_RX_MODE_ADDRESS_FILTER | RADIO_RX_MODE_AUTOACK);
 * ```
 * @{
 */

/**
 * Enable address-based frame filtering.
 *
 * This will typically involve filtering based on PAN ID, Short address and
 * long address. The filtering will consider the params RADIO_PARAM_PAN_ID,
 * RADIO_PARAM_16BIT_ADDR and RADIO_PARAM_64BIT_ADDR respectively.
 */
#define RADIO_RX_MODE_ADDRESS_FILTER   (1 << 0)

/**
 * Enable automatic transmission of ACK frames
 */
#define RADIO_RX_MODE_AUTOACK          (1 << 1)

/**
 * Enable/disable/get the state of radio driver poll mode operation
 */
#define RADIO_RX_MODE_POLL_MODE        (1 << 2)
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * Radio TX mode control / retrieval
 *
 * The radio transmission mode controls whether transmissions should
 * be done using clear channel assessment (if supported by the
 * radio). If send-on-CCA is enabled, the radio's send function will
 * wait for a radio-specific time window for the channel to become
 * clear. If this does not happen, the send function will return
 * `RADIO_TX_COLLISION`.
 */
#define RADIO_TX_MODE_SEND_ON_CCA      (1 << 0)

/**
 * Radio return values when setting or getting radio parameters.
 */
typedef enum radio_result_e {
  RADIO_RESULT_OK, /**< The parameter was set/read successfully */
  RADIO_RESULT_NOT_SUPPORTED, /**< The parameter is not supported */
  RADIO_RESULT_INVALID_VALUE, /**< The `value` argument was incorrect */

  /**
   * An error occurred when getting/setting the parameter, but the arguments
   * were otherwise correct.
   */
  RADIO_RESULT_ERROR
} radio_result_t;

/**
 * Radio return values for the `transmit()` function.
 */
enum radio_tx_e {
  /**
   * TX was successful and where an ACK was requested one was received
   */
  RADIO_TX_OK,

  /**
   * An error occurred during transmission.
   *
   * This will typically signify that the transmitted frame was too long/short
   * or that an error occurred at the radio driver level.
   */
  RADIO_TX_ERR,

  /**
   * TX failed due to a collision
   */
  RADIO_TX_COLLISION,

  /**
   * A unicast frame was sent OK but an ACK was _not_ received
   */
  RADIO_TX_NOACK,
};
/*---------------------------------------------------------------------------*/
/**
 * \name The Contiki-NG RF driver API
 * @{
 */
/**
 * The structure of a Contiki-NG radio device driver.
 *
 * Typically this data structure will represent the driver of an IEEE
 * 802.15.4-compliant radio hardware.
 *
 * This data structure is the only required interface between the radio driver
 * and the Contiki-NG network stack. All functions implemented in the radio
 * driver, including those pointed to by the fields of this structure can
 * be static.
 */
struct radio_driver {

  /**
   * Initialise the radio hardware.
   *
   * \retval 1 Initialisation successful
   * \retval 0 Initialisation failed
   *
   * This function will be called once during boot. It shall perform one-off
   * initialisation of the radio driver and hardware. Typical operations to
   * implement as part of this function are initialisation of driver internal
   * data structures and initial configuration of the radio hardware.
   *
   * This function is expected to apply configuration that persists across
   * radio on/off cycles. Non-persistent changes should be implemented as part
   * of `on()` instead.
   *
   * This function may, but is not strictly expected to put the radio in RX mode.
   * The Contiki-NG boot sequence will put the radio in RX mode explicitly by
   * a subsequent call to `on()`.
   */
  int (* init)(void);

  /**
   * Prepare the radio with a packet to be sent.
   *
   * \param payload A pointer to the location of the packet
   * \param payload_len The length of the packet to be sent
   * \retval 0 Packet copied successfully
   * \retval 1 The packet could not be copied
   *
   * This function is expected to copy `payload_len` bytes from the location
   * pointed to by `payload` to a location internal to the radio driver. In a
   * typical scenario this will be a separate buffer in RAM, or the radio
   * hardware's FIFO.
   *
   * `payload` will contain the MAC header (MHR) and MAC payload, but it
   * will _not_ contain the physical header or the MAC footer (MFR).
   *
   * `payload_len` must be lower than or equal to the value retrieved when calling
   * NETSTACK_RADIO.get_value(RADIO_CONST_MAX_PAYLOAD_LEN, ...)
   *
   * This function will return an error if the radio driver could not copy
   * the packet to a location internal to the driver. Commonly this may happen
   * if the latter is occupied by a previous packet which has yet to be sent.
   */
  int (* prepare)(const void *payload, unsigned short payload_len);

  /**
   * Send the packet that has previously been prepared.
   *
   * \param transmit_len The number of bytes to transmit
   * \return This function will return one of the radio_tx_e enumerators
   *
   * The radio driver is not expected to remember the packet even if TX fails.
   *
   * `transmit_len` is equal the length of a previously prepared packet.
   * Semantically it is identical to the `payload_len` argument of the
   * `prepare()` function.
   *
   * A previously prepared packet shall contain the MAC header (MHR) and MAC
   * payload, but it shall _not_ contain the physical header or the MAC footer
   * (MFR). This function shall make sure that all necessary physical layer
   * symbols are transmitted before the packet. In the case of .15.4 radios
   * this includes the synch header (SHR), preamble and physical header (PHR).
   * This function shall also make sure that the MFR is transmitted.
   *
   * Unless an error occurs, this function will wait until the packet has
   * been fully transmitted.
   *
   * If `RADIO_PARAM_TX_MODE & RADIO_TX_MODE_SEND_ON_CCA` then this function
   * should perform a CCA before transmission. If this CCA fails the function
   * shall return `RADIO_TX_COLLISION`.
   *
   * This function may be called while the radio is powered-off, or while the
   * radio is in RX mode. In the former case, it shall power on the radio and
   * enter TX mode, ideally bypassing RX mode to reduce off->TX turnaround
   * time. In the latter case, the function shall perform an RX-TX transition.
   *
   * This function may leave the radio in RX mode after transmission, but this
   * is not necessary since the caller will explicitly request the correct
   * radio state after this function returns: This will either be a request to
   * revert to RX mode by a call to `on()`, or a request to power the radio
   * down by a call to `off()`.
   */
  int (* transmit)(unsigned short transmit_len);

  /**
   * Prepare & transmit a packet.
   *
   * \param payload A pointer to the location of the packet
   * \param payload_len The length of the packet to be sent
   * \return This function will return one of the radio_tx_e enumerators
   *
   * This function shall behave exactly as a call to `prepare()`, immediately
   * followed by a call to `transmit()`.
   */
  int (* send)(const void *payload, unsigned short payload_len);

  /**
   * Read a received packet into a buffer.
   *
   * \param buf A pointer the the buffer where the packet is to be copied
   * \param buf_len The length of `buf`
   * \return The number of bytes copied to `buf`
   *
   * If the radio has no correctly-received packets then this function will
   * return 0.
   *
   * The buffer `buf` will be allocated by the caller.
   *
   * The radio driver is not expected to remember the packet after this call
   * returns.
   *
   * This function is expected to be able to deliver a packet to the MAC layer
   * even if the radio is powered down by a call to `off()`.
   *
   * When this function returns, `buf` will contain the MAC header (MHR) and
   * MAC payload, but it will _not_ contain the physical header or the MAC
   * footer (MFR).
   */
  int (* read)(void *buf, unsigned short buf_len);

  /**
   * Perform a Clear-Channel Assessment (CCA) to find out if there is
   * a packet in the air or not.
   *
   * \retval 0 The channel is busy
   * \retval 1 The channel is clear
   *
   * It is up to the radio driver's developer to decide how the CCA will be
   * performed. Some radios have built-in, .15.4-compliant CCA operation; for
   * those radios, it is up to the developer to decide which CCA mode to use.
   *
   * This function should not be called while the radio is not in RX mode. If
   * this happens, this function shall return 0 and it will _not_ try to
   * power-on the radio in order to perform a CCA.
   */
  int (* channel_clear)(void);

  /**
   * Check if the radio driver is currently receiving a packet.
   *
   * \retval 1 Reception of a packet is in progress
   * \retval 0 No reception in progress
   *
   * If at the point of calling this function the radio is not in RX mode, for
   * example as a result of a previous call to `off()`, this function will
   * immediately return 0.
   */
  int (* receiving_packet)(void);

  /**
   * Check if a packet has been received and is available in the radio driver's
   * buffers.
   *
   * \retval 1 One (or more) packet(s) is (are) available
   * \retval 0 No packets available
   *
   * This function may be called while the radio is powered down by a previous
   * call to `off()`. If that happens, the function shall not power on the
   * radio.
   */
  int (* pending_packet)(void);

  /**
   * Turn the radio on.
   *
   * \retval 1 The call was successful and the radio is now in RX mode
   * \retval 0 The call failed
   *
   * This function will put the radio in a state ready to receive packets. The
   * function will power-on and configure the radio if necessary.
   *
   * This function shall not intentionally discard any previously received
   * packets.
   */
  int (* on)(void);

  /**
   * Turn the radio off.
   *
   * \retval 1 Success
   * \retval 0 Error
   *
   * This function shall put the radio to its lowest power consumption state.
   *
   * This function may be called immediately after the reception of a packet,
   * but before this packet gets copied to the upper layers through a call to
   * `read()`. If powering down the radio would result in the received packet
   * getting lost, due to e.g. non-retention of the radio's hardware FIFO,
   * then the radio driver shall make sure any received packets get copied to
   * RAM first. This function shall not intentionally discard any previously
   * received packets.
   */
  int (* off)(void);

  /**
   * Get a radio parameter value.
   *
   * \param param The parameter to retrieve: An enumerator of `radio_param_e`
   * \param value A pointer to store the value of `param`
   * \return An enumerator of `radio_result_t`
   *
   * This function shall copy the current value of parameter `param` to the
   * location pointed to by `value`. The caller shall allocate `value`.
   */
  radio_result_t (* get_value)(radio_param_t param, radio_value_t *value);

  /**
   * Set a radio parameter value.
   *
   * \param param The parameter to set: An enumerator of `radio_param_e`
   * \param value The new value for `param`
   * \return An enumerator of `radio_result_t`
   *
   * This function shall set the value of a radio parameter.
   *
   * If this function is called while the radio is powered on, the requested
   * change shall take effect immediately. If the radio is powered-off, the
   * change shall take effect in the next power-on cycle.
   */
  radio_result_t (* set_value)(radio_param_t param, radio_value_t value);

  /**
   * Get a radio parameter object.
   *
   * \param param The parameter to retrieve: An enumerator of `radio_param_e`
   * \param dest A pointer to a buffer where the value of `param` shall be stored
   * \param size The size of the `dest` buffer
   * \return An enumerator of `radio_result_t`
   *
   * The argument `dest` must point to a memory area of at least `size` bytes,
   * and this memory area will contain the parameter object if the function
   * succeeds. `dest` shall be allocated by the caller.
   */
  radio_result_t (* get_object)(radio_param_t param, void *dest, size_t size);

  /**
   * Set a radio parameter object.
   *
   * \param param The parameter to set: An enumerator of `radio_param_e`
   * \param src A pointer to a buffer where the new value is stored
   * \param size The size of the `src` buffer
   * \return An enumerator of `radio_result_t`
   *
   * The memory area referred to by the argument `src` will not be accessed
   * after the function returns.
   *
   * If this function is called while the radio is powered on, the requested
   * change shall take effect immediately. If the radio is powered-off, the
   * change shall take effect in the next power-on cycle.
   */
  radio_result_t (* set_object)(radio_param_t param, const void *src,
                                size_t size);
};
/** @} */
/*---------------------------------------------------------------------------*/
#endif /* RADIO_H_ */
/*---------------------------------------------------------------------------*/
/** @} */
/** @} */

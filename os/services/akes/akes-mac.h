/*
 * Copyright (c) 2015, Hasso-Plattner-Institut.
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
 * \addtogroup akes
 * @{
 *
 * \file
 *         Special MAC driver and special FRAMER.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef AKES_MAC_H_
#define AKES_MAC_H_

#include "contiki.h"
#include "akes/akes-nbr.h"
#include "lib/aes-128.h"
#include "lib/ccm-star.h"
#include "net/mac/llsec802154.h"
#include "net/mac/mac.h"
#include "sys/cc.h"
#include <stdbool.h>

#ifdef AKES_MAC_CONF_ENABLED
#define AKES_MAC_ENABLED AKES_MAC_CONF_ENABLED
#else /* AKES_MAC_CONF_ENABLED */
#define AKES_MAC_ENABLED (0)
#endif /* AKES_MAC_CONF_ENABLED */

#ifdef AKES_MAC_CONF_DECORATED_FRAMER
#define AKES_MAC_DECORATED_FRAMER AKES_MAC_CONF_DECORATED_FRAMER
#else /* AKES_MAC_CONF_DECORATED_FRAMER */
#define AKES_MAC_DECORATED_FRAMER csl_framer
#endif /* AKES_MAC_CONF_DECORATED_FRAMER */

#ifdef AKES_MAC_CONF_DECORATED_MAC
#define AKES_MAC_DECORATED_MAC AKES_MAC_CONF_DECORATED_MAC
#else /* AKES_MAC_CONF_DECORATED_MAC */
#define AKES_MAC_DECORATED_MAC csl_driver
#endif /* AKES_MAC_CONF_DECORATED_MAC */

#ifdef AKES_MAC_CONF_UNICAST_SEC_LVL
#define AKES_MAC_UNICAST_SEC_LVL AKES_MAC_CONF_UNICAST_SEC_LVL
#else /* AKES_MAC_CONF_UNICAST_SEC_LVL */
#define AKES_MAC_UNICAST_SEC_LVL (6)
#endif /* AKES_MAC_CONF_UNICAST_SEC_LVL */

_Static_assert((AKES_MAC_UNICAST_SEC_LVL >= 1)
               && (AKES_MAC_UNICAST_SEC_LVL <= 7)
               && (AKES_MAC_UNICAST_SEC_LVL != 4),
               "security levels must be in {1, 2, 3, 5, 6, 7}");

#ifdef AKES_MAC_CONF_BROADCAST_SEC_LVL
#define AKES_MAC_BROADCAST_SEC_LVL AKES_MAC_CONF_BROADCAST_SEC_LVL
#else /* AKES_MAC_CONF_BROADCAST_SEC_LVL */
#define AKES_MAC_BROADCAST_SEC_LVL AKES_MAC_UNICAST_SEC_LVL
#endif /* AKES_MAC_CONF_BROADCAST_SEC_LVL */

_Static_assert((AKES_MAC_BROADCAST_SEC_LVL >= 1)
               && (AKES_MAC_BROADCAST_SEC_LVL <= 7)
               && (AKES_MAC_BROADCAST_SEC_LVL != 4),
               "security levels must be in {1, 2, 3, 5, 6, 7}");

#define AKES_MAC_UNICAST_MIC_LEN \
  LLSEC802154_MIC_LEN(AKES_MAC_UNICAST_SEC_LVL)
#define AKES_MAC_BROADCAST_MIC_LEN \
  LLSEC802154_MIC_LEN(AKES_MAC_BROADCAST_SEC_LVL)
#define AKES_MAC_MIN_MIC_LEN \
  MIN(AKES_MAC_BROADCAST_MIC_LEN, AKES_MAC_UNICAST_MIC_LEN)

#ifdef AKES_MAC_CONF_STRATEGY
#define AKES_MAC_STRATEGY AKES_MAC_CONF_STRATEGY
#else /* AKES_MAC_CONF_STRATEGY */
#define AKES_MAC_STRATEGY csl_strategy
#endif /* AKES_MAC_CONF_STRATEGY */

#ifdef AKES_MAC_CONF_UNSECURE_UNICASTS
#define AKES_MAC_UNSECURE_UNICASTS AKES_MAC_CONF_UNSECURE_UNICASTS
#else /* AKES_MAC_CONF_UNSECURE_UNICASTS */
#define AKES_MAC_UNSECURE_UNICASTS (1)
#endif /* AKES_MAC_CONF_UNSECURE_UNICASTS */

enum akes_mac_verify_result {
  AKES_MAC_VERIFY_RESULT_SUCCESS,
  AKES_MAC_VERIFY_RESULT_INAUTHENTIC,
  AKES_MAC_VERIFY_RESULT_REPLAYED
};

/**
 * Structure of a strategy regarding compromise resilience
 */
struct akes_mac_strategy {

  /**
   * \brief Initializes the strategy.
   */
  void (* init)(void);

  /**
   * \brief         Sets the CCM* nonce for the packetbuf's frame.
   * \param nonce   The CCM* nonce.
   * \param forward Tells whether the frame is outgoing or incoming.
   */
  void (* generate_nonce)(uint8_t nonce[static CCM_STAR_NONCE_LENGTH],
                          bool forward);

  /**
   * \brief      Secures and sends the packetbuf's frame.
   * \param sent The callback to invoke after transmission.
   * \param ptr  A pointer to pass to the callback.
   */
  void (* send)(mac_callback_t sent, void *ptr);

  /**
   * \brief  Called before creating the packetbuf's frame.
   * \retval true  On success.
   * \retval false On error.
   */
  bool (* before_creation)(void);

  /**
   * \brief        Called when the packetbuf's frame has been created.
   * \retval true  On success.
   * \retval false On error.
   */
  bool (* after_creation)(void);

  /**
   * \brief        Unsecures the packetbuf's frame and checks its freshness.
   * \param sender The sender.
   * \return       A result code.
   */
  enum akes_mac_verify_result (* verify)(akes_nbr_t *sender);

  /**
   * \brief  Determines how many bytes will be added to the packetbuf's frame.
   * \return The number bytes that will be added.
   */
  uint_fast8_t (* get_overhead)(void);

  /**
   * \brief        Adds piggyback data to HELLOs, HELLOACKs, ACKs, and UPDATEs.
   * \param data   The place to put the piggyback data.
   * \param cmd_id The command frame identifier.
   * \return       A pointer to the byte after the written piggyback data.
   */
  uint8_t *(* write_piggyback)(uint8_t *data,
                               uint8_t cmd_id,
                               akes_nbr_entry_t *entry);

  /**
   * \brief        Parses piggyback data of HELLOs, HELLOACKs, ACKs, and
   *               UPDATEs.
   * \param data   The piggyback data.
   * \param cmd_id The command frame identifier.
   * \param entry  The sender's entry.
   * \param meta   Metadata if applicable and \c NULL otherwise.
   * \param data   A pointer to the byte after the read piggyback data.
   */
  const uint8_t *(* read_piggyback)(const uint8_t *data,
                                    uint8_t cmd_id,
                                    const akes_nbr_entry_t *entry,
                                    const akes_nbr_tentative_t *meta);

  /**
   * \brief     Called after a HELLOACK has been sent to a neighbor.
   * \param nbr The neighbor.
   */
  void (* on_helloack_sent)(akes_nbr_t *nbr);

  /**
   * \brief Called when a fresh authentic HELLO has ben received.
   */
  void (* on_fresh_authentic_hello)(void);

  /**
   * \brief Called when a fresh authentic HELLOACK has been received.
   */
  void (* on_fresh_authentic_helloack)(void);
};

extern const struct framer AKES_MAC_DECORATED_FRAMER;
extern const struct mac_driver AKES_MAC_DECORATED_MAC;
extern const struct akes_mac_kes AKES_MAC_KES;
extern const struct akes_mac_strategy AKES_MAC_STRATEGY;
extern const struct mac_driver akes_mac_driver;
extern const struct framer akes_mac_framer;
#if AKES_NBR_WITH_GROUP_KEYS
extern uint8_t akes_mac_group_key[AES_128_KEY_LENGTH];
#endif /* AKES_NBR_WITH_GROUP_KEYS */

/**
 * \brief               Reports on L2 traffic to the routing protocol.
 * \param status        The transmission status (see os/net/mac/mac.h).
 * \param transmissions The total number of transmission attempts.
 */
void akes_mac_report_to_network_layer(int status, int transmissions);

/**
 * \brief               Reports on L2 traffic to the routing protocol.
 * \param address       The destination address.
 * \param status        The transmission status (see os/net/mac/mac.h).
 * \param transmissions The total number of transmission attempts.
 */
void akes_mac_report_to_network_layer_with_address(const linkaddr_t *address,
                                                   int status,
                                                   int transmissions);

/**
 * \brief          Sets deduplication- and security-related packetbuf
 *                 attributes.
 * \param receiver The receiver.
 */
void akes_mac_set_numbers(akes_nbr_t *receiver);

/**
 * \brief        Checks whether the packetbuf's frame was received already.
 * \param sender The sender.
 * \return       \c true if it is a duplicate and \c false otherwise.
 */
bool akes_mac_received_duplicate(akes_nbr_t *sender);

/**
 * \brief  Tells whether the packetbuf's frame is a HELLO.
 * \return \c true if it is a HELLO and \c false otherwise.
 */
bool akes_mac_is_hello(void);

/**
 * \brief  Tells whether the packetbuf's frame is a HELLOACK.
 * \return \c true if it is a HELLOACK and \c false otherwise.
 */
bool akes_mac_is_helloack(void);

/**
 * \brief  Tells whether the packetbuf's frame is an ACK.
 * \return \c true if it is an ACK and \c false otherwise.
 */
bool akes_mac_is_ack(void);

/**
 * \brief               Tells if a dispatch byte belongs to a HELLO, HELLOACK,
 *                      or ACK.
 * \param dispatch_byte The dispatch byte.
 * \return              \c true if the dispatch byte belongs to a a HELLO,
 *                      HELLOACK, or ACK, and \c false otherwise.
 */
bool akes_mac_is_hello_helloack_or_ack(uint8_t dispatch_byte);

/**
 * \brief  Tells whether the packetbuf's frame is an UPDATE.
 * \return \c true if it is an UPDATE and \c false otherwise.
 */
bool akes_mac_is_update(void);

/**
 * \brief  Returns the appropriate security level for the packetbuf's frame.
 * \return The appropriate security level.
 */
uint_fast8_t akes_mac_get_sec_lvl(void);

/**
 * \brief  Unsecures the packetbuf's frame and checks its freshness.
 * \return The sender's entry or \c NULL on error.
 */
akes_nbr_entry_t *akes_mac_check_frame(void);

/**
 * \brief  Determines the appropriate MIC length for the packetbuf's frame.
 * \return The appropriate MIC length.
 */
uint_fast8_t akes_mac_mic_len(void);

/**
 * \brief               Secures or unsecures the packetbuf's frame.
 * \param key           The symmetric key to use.
 * \param shall_encrypt Tells if encryption or decryption shall be done or not.
 * \param mic           The resulting or received message integrity code.
 * \param forward       Switches between securing and unsecuring.
 * \retval true         Securing succeeded or the frame turned out authentic.
 * \retval false        Securing failed or the frame turned out inauthentic.
 */
bool akes_mac_aead(const uint8_t key[static AES_128_KEY_LENGTH],
                   bool shall_encrypt,
                   uint8_t mic[static AKES_MAC_MIN_MIC_LEN],
                   bool forward);

/**
 * \brief        Decrypts and checks the authenticity of the packetbuf's frame.
 * \param  key   The symmetric key to use.
 * \retval true  The frame turned out authentic.
 * \retval false The frame turned out inauthentic.
 */
bool akes_mac_unsecure(const uint8_t *key);

#if MAC_CONF_WITH_CSMA
void akes_mac_input_from_csma(void);
#endif /* MAC_CONF_WITH_CSMA */

#endif /* AKES_MAC_H_ */

/** @} */

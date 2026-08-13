/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden AB
 * Copyright (c) 2020, Industrial Systems Institute (ISI), Patras, Greece
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
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * \file
 *      EDHOC server API [RFC9528] with CoAP Block-Wise Transfer [RFC7959]
 * \author
 *      Lidia Pocero <pocero@isi.gr>, Peter A Jonsson, Rikard Höglund, Marco Tiloca
 *      Christos Koulamas <cklm@isi.gr>
 */

/**
 * \addtogroup edhoc
 * @{
 */

#ifndef EDHOC_SERVER_H_
#define EDHOC_SERVER_H_

#include "coap-engine.h"
#include "edhoc.h"
#include "coap-timer.h"
#include "coap-block1.h"
#include "edhoc-exporter.h"

/**
 * \brief Time limit value for EDHOC protocol completion
 */
#ifdef EDHOC_CONF_TIMEOUT
#define SERV_TIMEOUT_VAL EDHOC_CONF_TIMEOUT
#else
#define SERV_TIMEOUT_VAL 10000
#endif

/* EDHOC process states */
#define SERV_HANDSHAKE_COMPLETE 1
#define SERV_HANDSHAKE_RESET 2

/**
 * \brief CoAP resource
 */
extern coap_resource_t res_edhoc;

/**
 * \brief EDHOC Server Struct
 */
typedef struct edhoc_server {
  uint16_t con_num;
  uint8_t state;
  bool rx_msg1;
  bool rx_msg3;
  uip_ipaddr_t con_ipaddr;
} edhoc_server_t;

/**
 * \brief EDHOC server Application data struct
 */
typedef struct edhoc_server_ad {
  char ad_1[EDHOC_MAX_AD_SZ];
  uint8_t ad_1_sz;
  char ad_2[EDHOC_MAX_AD_SZ];
  uint8_t ad_2_sz;
  char ad_3[EDHOC_MAX_AD_SZ];
  uint8_t ad_3_sz;
} edhoc_server_ad_t;

/**
 * \brief EDHOC server data event struct
 */
typedef struct ecc_data_event {
  uint8_t val;
  edhoc_server_ad_t ad;
} ecc_data_event_t;

/**
 * \brief Activate the EDHOC CoAP Resource
 *
 *  Activate the EDHOC well-known CoAP Resource at the Uri-Path defined
 *  in the WELL_KNOW macro.
 */
void edhoc_server_init(void);

/**
 * \brief Create a new EDHOC context for a new EDHOC protocol session
 * \retval Non-zero if the authentication credentials for the EDHOC server exist in the key-storage
 *         and the EDHOC server starts correctly.
 *
 * This function retrieves the DH-static authentication key pair of the Server from the edhoc-key-storage.
 * The authentication keys must be established in the EDHOC key storage before running the EDHOC protocol.
 * Creates a new EDHOC context and generates the DH-ephemeral key for the specific session.
 * A new EDHOC protocol session must be created for each new EDHOC client connection attempt.
 */
uint8_t edhoc_server_start(void);

/**
 * \brief Reset the EDHOC handshake state for a new client connection
 * \retval Non-zero if the authentication credentials for the EDHOC server exist in the key-storage
 *         and the EDHOC server resets correctly.
 *
 * Resets the EDHOC handshake state to prepare for a new client handshake.
 * This clears the current session state while keeping the server running.
 * Must be called after completing or aborting a handshake before accepting the next client.
 */
uint8_t edhoc_server_reset_handshake(void);

/**
 * \brief Check if an EDHOC server session has finished
 * \param ev Process event
 * \param data Process data
 * \retval SERV_HANDSHAKE_COMPLETE (non-zero) if the session finished successfully with an EDHOC client
 * \retval SERV_HANDSHAKE_RESET (non-zero) if the handshake was reset
 * \retval 0 if the event is not from the EDHOC server or the handshake has not finished yet
 *
 * This function checks the events triggered from the EDHOC server protocol, reporting both
 * handshake completion (SERV_HANDSHAKE_COMPLETE) and handshake reset (SERV_HANDSHAKE_RESET).
 * A non-zero return does not by itself imply success, so callers must distinguish the two values.
 */
int8_t edhoc_server_callback(process_event_t ev, void *data);

/**
 * \brief Close the EDHOC context
 *
 * This function must be called after the Security Context is exported to free the
 * allocated memory.
 */
void edhoc_server_close(void);

/**
 * \brief Run the EDHOC Responder role process
 * \param req The request CoAP message received
 * \param res The response CoAP message to send back
 * \param ser The EDHOC server struct
 * \param msg A pointer to the buffer with the received message
 * \param len The received message length
 *
 * This function must be called from a CoAP POST handler to run the EDHOC protocol Responder
 * role. EDHOC messages 1 and 3 are transferred in POST requests and EDHOC message 2
 * is transferred in 2.04 (Changed) responses.
 */
void edhoc_server_process(coap_message_t *req, coap_message_t *res, edhoc_server_t *ser, uint8_t *msg, size_t len);

/**
 * \brief Set the Application Data to be carried in EDHOC message 2
 * \param buf A pointer to a buffer that contains the Application data to be copied
 * \param buf_sz The Application data length
 *
 * This function sets the Application data to be carried in EDHOC message 2.
 */
void edhoc_server_set_ad_2(const void *buf, uint8_t buf_sz);

/**
 * \brief Get the Application Data received in EDHOC message 1
 * \param buf A pointer to a buffer to copy the Application data
 * \return ad_sz The Application data length
 *
 * This function copies the Application data from the received EDHOC message 1 to the buffer.
 */
uint8_t edhoc_server_get_ad_1(char *buf);

/**
 * \brief Get the Application Data received in EDHOC message 3
 * \param buf A pointer to a buffer to copy the Application data
 * \return ad_sz The Application data length
 *
 * This function copies the Application data from the received EDHOC message 3 to the buffer.
 */
uint8_t edhoc_server_get_ad_3(char *buf);

#endif /* EDHOC_SERVER_H_ */
/** @} */

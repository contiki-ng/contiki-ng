/*
 * Copyright (c) 2022, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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
 * \file
 *      DTLS (Mbed TLS implementation) support for CoAP
 * \author
 *      Jayendra Ellamathy <ejayen@gmail.com>
 */

#ifndef MBEDTLS_SUPPORT_H_
#define MBEDTLS_SUPPORT_H_

#include MBEDTLS_CONFIG_FILE
#include "mbedtls/ssl.h"
#if !defined(MBEDTLS_NO_PLATFORM_ENTROPY)
#include "mbedtls/entropy.h"
#endif /* !MBEDTLS_NO_PLATFORM_ENTROPY */
#include "mbedtls/ctr_drbg.h"

/*TODO: agree on config structure! */
#ifdef MBEDTLS_TIMING_ALT
#include "timing_alt.h"
#else
#include "mbedtls/timing.h"
#endif

#ifdef COAP_DTLS_CONF_WITH_CERT
#include "mbedtls/x509.h"
#endif /* COAP_DTLS_CONF_WITH_CERT */
#ifdef COAP_DTLS_CONF_WITH_SERVER
#include "mbedtls/ssl_cookie.h"
#if defined(MBEDTLS_SSL_CACHE_C)
#include "mbedtls/ssl_cache.h"
#endif /* MBEDTLS_SSL_CACHE_C */
#endif /* COAP_DTLS_CONF_WITH_SERVER */

#include "dtls-support-config.h"

#include "coap-endpoint.h"
#include "coap-keystore.h"

typedef enum coap_dtls_sec_mode_e {
  COAP_DTLS_SEC_MODE_NONE = 0,
  COAP_DTLS_SEC_MODE_PSK = 1,
  COAP_DTLS_SEC_MODE_CERT = 2,
} coap_dtls_sec_mode_t;

typedef enum coap_mbedtls_role_e {
  COAP_MBEDTLS_ROLE_NONE = 0,
  COAP_MBEDTLS_ROLE_CLIENT = 1,
  COAP_MBEDTLS_ROLE_SERVER = 2,
} coap_mbedtls_role_t;

typedef enum coap_mbedtls_event_e {
  COAP_MBEDTLS_EVENT_NONE = 0,
  COAP_MBEDTLS_EVENT_RETRANSMISSION_EVENT = 1,
  COAP_MBEDTLS_EVENT_SEND_MESSAGE_EVENT = 2,
} coap_mbedtls_event_t;

/* DTLS session info -- config, current state, etc */
typedef struct coap_dtls_session_info {
  struct coap_dtls_session_info *next;
  enum coap_mbedtls_role_e role;
  coap_endpoint_t ep; /* Server/Client address when role
                         is Client/Server respectively */
  bool is_packet_consumed; /* To prevent Mbed TLS from reading
                              the same packet more than once. */
  mbedtls_ssl_context ssl;
  mbedtls_ssl_config conf;
  uint32_t ciphersuite;
#if !defined(MBEDTLS_NO_PLATFORM_ENTROPY)
  mbedtls_entropy_context entropy;
#endif /* !MBEDTLS_NO_PLATFORM_ENTROPY */
  mbedtls_ctr_drbg_context ctr_drbg;
  struct mbedtls_timing_delay_context timer;
  struct etimer retransmission_et; /* Event timer to call the handshake function
                                      for re-transmssion */
#ifdef COAP_DTLS_CONF_WITH_CERT
  char *hostname; /* Used for SNI (Server Name Indication) */
  mbedtls_x509_crt ca_cert; /* Root CA certificate */
  mbedtls_x509_crt own_cert; /* Our (Client/Server) certificate */
  mbedtls_pk_context pkey; /* Our (Client/Server) private key */
#endif /* COAP_DTLS_CONF_WITH_CERT */
#ifdef COAP_DTLS_CONF_WITH_SERVER
  bool in_use; /* Determines if this server session is in use by a client. */
  mbedtls_ssl_cookie_ctx cookie_ctx;
#if defined(MBEDTLS_SSL_CACHE_C)
  mbedtls_ssl_cache_context cache;
#endif /* MBEDTLS_SSL_CACHE_C */
#endif /* COAP_DTLS_CONF_WITH_SERVER */
} coap_dtls_session_info_t;

/* DTLS message info */
typedef struct coap_dtls_send_message {
  struct coap_mbedtls_send_message *next;
  coap_endpoint_t ep;
  unsigned char send_buf[COAP_MBEDTLS_MTU];
  size_t len;
} coap_dtls_send_message_t;

/* Struct stores global DTLS info */
typedef struct coap_dtls_context {
  struct etimer fragmentation_et;
  LIST_STRUCT(sessions); /* List of DTLS sessions */
  LIST_STRUCT(send_message_fifo); /* DTLS message to send queue */
  struct uip_udp_conn *udp_conn; /* DTLS will listen on this udp port */
  struct process *host_process; /* Process which will take care of sending
                                              DTLS messages -- CoAP UIP process */
  bool ready; /* Determines whether DTLS is initialized and ready. */
} coap_dtls_context_t;

/**
 * \brief     Initializes CoAP-MbedTLS global info. Must be the first thing that is
 *            called before using CoAP-MbedTLS.
 */
void coap_dtls_init(void);

/**
 * \brief     Handler for timer, and process-poll events.
 *            Must be called by the host process (CoAP Engine).
 */
void coap_dtls_event_handler(void);

/**
 * \brief    Registers, 1. UDP port info. 2. Host process (Coap Engine).
 *
 * \param udp_conn  Pointer to UDP port information. This will be used when
 *                  CoAP-MbedTLS needs to send messages via UDP.
 *
 * \param host_process  Pointer to the host process. This process will recieve
 *                      a poll event when a DTLS message needs to be sent.
 */
void coap_dtls_conn_init(struct uip_udp_conn *udp_conn,
                         struct process *host_process);

/**
 * \brief    Encrypt app. data and send via UDP.
 *
 * \param ep  Pointer to destination CoAP endpoint.
 * \param message  Pointer to the buffer holding app. data.
 * \param len  Length of message to be sent.
 *
 * \return  SUCCESS: Number of bytes written.
 *          FAILURE: -1
 */
int coap_ep_dtls_write(const coap_endpoint_t *ep,
                       const unsigned char *message, int len);

/**
 * \brief    Handler for new DTLS messages. Handles both handshake and decryption
 *           of record layer messages.
 *
 * \param ep  Pointer of source CoAP endpoint.
 *
 * \return  SUCCESS: 0 for Handshake; Number of bytes read for record layer packet.
 *          FAILURE: -1
 */
int coap_ep_dtls_handle_message(const coap_endpoint_t *ep);

/**
 * \brief   Disconnect a peer. Sends a close notification message to peer.
 *          Followed by cleanup of session struct, free memory.
 *
 * \param ep  Pointer of peer CoAP endpoint.
 */
void coap_ep_dtls_disconnect(const coap_endpoint_t *ep);

/**
 * \brief   Get session struct associated with CoAP endpoint.
 *
 * \param ep  Pointer of peer CoAP endpoint.
 */
coap_dtls_session_info_t *
coap_ep_get_dtls_session_info(const coap_endpoint_t *ep);

#ifdef COAP_DTLS_CONF_WITH_CLIENT
/**
 * \brief   Connect to a DTLS server. To be used by the client.
 *          Sets up a client session and initiates the handshake.
 *
 * \param ep  Pointer of peer CoAP endpoint.
 * \param sec_mode  Enum representing mode of security: certificates or PSK.
 * \param keystore_entry  Pointer to PSK or certificate info struct. Will be
 *                        type-casted internall based on sec_mode enum.
 *
 * \return  SUCCESS: 1
 *          FAILURE: -1
 */
int coap_ep_dtls_connect(const coap_endpoint_t *ep,
                         coap_dtls_sec_mode_t sec_mode, const void *keystore_entry);
#endif /* COAP_DTLS_CONF_WITH_CLIENT */

#ifdef COAP_DTLS_CONF_WITH_SERVER
/**
 * \brief   Setup a DTLS server session. Must be done before any client connection
 *          can be accepted.
 *
 * \param ep  Pointer of peer CoAP endpoint.
 * \param sec_mode  Enum representing mode of security: certificates or PSK.
 * \param keystore_entry  Pointer to PSK or certificate info struct. Will be
 *                        type-casted internall based on sec_mode enum.
 *
 * \return  SUCCESS: 1
 *          FAILURE: -1
 */
int coap_dtls_server_setup(const coap_dtls_sec_mode_t sec_mode,
                           const void *keystore_entry);
#endif /* COAP_DTLS_CONF_WITH_SERVER */

/**
 * \brief  Check if a CoAP endpoint is a peer in the list of DTLS sessions
 *
 * \param ep  Pointer of peer CoAP endpoint.
 *
 * \return  SUCCESS: true
 *          FAILURE: false
 */
bool coap_ep_is_dtls_peer(const coap_endpoint_t *ep);

/**
 * \brief  Check if a peer has completed the handshake successfully
 *
 * \param ep  Pointer of peer CoAP endpoint.
 *
 * \return  SUCCESS: true
 *          FAILURE: false
 */
bool coap_ep_is_dtls_connected(const coap_endpoint_t *ep);

/**
 * \brief  Check in what DTLS state the peer is in.
 *
 * \param ep  Pointer of peer CoAP endpoint.
 * \warning   Uses deprecated mbedtls getters
 *
 * \return  enum mbedtls_ssl_states
 */
int coap_ep_get_dtls_state(const coap_endpoint_t *ep);

#endif /* MBEDTLS_SUPPORT_H_ */

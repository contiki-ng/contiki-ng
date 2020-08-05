/*
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
 */

/**
 * \file
 *      EDHOC server API [draft-ietf-lake-edhoc-01] with CoAP Block-Wise Transfer [RFC7959]
 * \author
 *      Lidia Pocero <pocero@isi.gr>
 *      Christos Koulamas <cklm@isi.gr>
 */

/**
 * \addtogroup edhoc 
 * @{
 */

#ifndef _EDHOC_SERVER_API_H_
#define _EDHOC_SERVER_API_H_
#include "coap-engine.h"
#include "edhoc.h"
#include "coap-timer.h"
#include "coap-block1.h"
#include "edhoc-exporter.h"

/**
 * \brief The max lenght of the EDHOC msg, as CoAP payload
 */
#ifdef EDHOC_CONF_MAX_PAYLOAD
#define MAX_DATA_LEN EDHOC_CONF_MAX_PAYLOAD
#else
#define MAX_DATA_LEN 64
#endif

/**
 * \brief The max lenght of the Aplication Data
 */
#ifdef EDHOC_CONF_MAX_AD_SZ
#define MAX_AD_SZ EDHOC_CONF_MAX_AD_SZ
#else
#define MAX_AD_SZ 16
#endif

/**
 * \brief Limite time value to EDHOC protocol finished 
 */
#ifdef EDHOC_CONF_TIMEOUT
#define SERV_TIMEOUT_VAL EDHOC_CONF_TIMEOUT
#else
#define SERV_TIMEOUT_VAL 10000
#endif

/**
 * \brief EDHOC resource urti-path
 */
#define WELL_KNOWN ".well-known/edhoc"

/**
 * \brief EDHOC context struct used in the EDHOC protocol
 */
edhoc_context_t *ctx;

/**
 * \brief CoAp resource 
 */
extern coap_resource_t res_edhoc;

/**
 * \brief EDHOC Server Struct 
 */
typedef struct edhoc_server_t {
  uint16_t con_num;
  uint8_t state;
  bool rx_msg1;
  bool rx_msg3;
  uip_ipaddr_t con_ipaddr;
}edhoc_server_t;

/**
 * \brief EDHOC server Application data struct
 */
typedef struct edhoc_server_ad_t {
  char ad_1[MAX_AD_SZ];
  uint8_t ad_1_sz;
  char ad_2[MAX_AD_SZ];
  uint8_t ad_2_sz;
  char ad_3[MAX_AD_SZ];
  uint8_t ad_3_sz;
} edhoc_server_ad_t;

/**
 * \brief EDHOC server data event struct
 */
typedef struct ecc_data_even_t {
  uint8_t val;
  edhoc_server_ad_t ad;
}ecc_data_even_t;

/**
 * \brief Activate the EDHOC CoAp Resource
 * 
 *  Activate the EDHOC well know CoAP Resource at the uri-path defined 
 *  on the WELL_KNOW macro.
 */
void edhoc_server_init();

/**
 * \brief Create a new EDHOC context for a new EDHOC protocol session
 *  This function gets the DH-static authentication pair keys of the Server from the edhoc-key-storage. 
 *  The authentication keys must be established at the EDHOC key storage before running the EDHOC protocol.
 *  Create a new EDHOC context and generate the DH-ephemeral key for the specific session.
 *  A new EDHOC protocol session must be created for each new EDHOC client try
 */
void edhoc_server_start();

/**
 * \brief Check if an EDHOC server session have finished
 * \param ev process event 
 * \param data process data
 * \retval non-zero if EDHOC session finished success with a EDHOC client
 * 
 *  This function checks the events trigger from the EDHOC server protocol looking for the 
 *  SERV_FINSHED event. 
 */
int8_t edhoc_server_callback(process_event_t ev, void *data);

/**
 * \brief Close the EDHOC context 
 * 
 * This function must be called after the Security Context is exported to liberate the 
 * allocated memory. 
 */
void edhoc_server_close();

/**
 * \brief run the EDHOC Responder part
 * \param request The request CoAp message received
 * \param response The response CoAp message to send back
 * 
 *  This function must be called from a CoAP response POST handler to run the EDHOC protocol Responder
 *  part. The EDHOC messages 1 and message 3 are transferred in POST requests and the EDHOC message 2
 *  is transferred in 2.04 (Changed) responses.  
 */
void edhoc_server_resource(coap_message_t *request, coap_message_t *response);


/**
 * \brief Set the Application Data to be carried on EDHOC message 2
 * \param buff A pointer to a buffer that contains the Application data to be copied
 * \param buff_sz The Application data length
 * 
 * This function set the Application data to be carried on EDHOC message 2
 */
void edhoc_server_set_ad_2(const void * buff, uint8_t buff_sz);

/**
 * \brief Get the Application Data received in EDHOC message 1
 * \param buff A pointer to a buffer to copy the Application data
 * \return ad_sz The Application data length
 * 
 * This function copy on the buff the Application data from the EDHOC message 1 received
 */
uint8_t edhoc_server_get_ad_1(char* buff);

/**
 * \brief Get the Application Data received in EDHOC message 3
 * \param buff A pointer to a buffer to copy the Application data
 * \return ad_sz The Application data length
 * 
 * This function copy on the buff the Application data from the EDHOC message 3 received
 */
uint8_t edhoc_server_get_ad_3(char* buff);
#endif /* _EDHOC_SERVER_API_H_ */
/** @} */
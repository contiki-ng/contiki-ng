/* SNMP protocol
 *
 * Copyright (C) 2008-2010  Robert Ernst <robert.ernst@linux-solutions.at>
 * Copyright (C) 2011       Javier Palacios <javiplx@gmail.com>
 * Copyright (C) 2019       Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See COPYING for GPL licensing information.
 */

#include "snmp.h"
#include "snmp_mib.h"
#include "snmp_asn1.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define LOG_MODULE "SNMP"
#define LOG_LEVEL LOG_LEVEL_SNMP

/*---------------------------------------------------------------------------*/
#define SNMP_VERSION_1_ERROR(resp, code, index) { \
    (resp)->error_status = code; \
    (resp)->error_index = index; \
    return 0; \
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define SNMP_VERSION_2_ERROR(resp, req, index, err) { \
    size_t len = (resp)->value_list_length; \
    memcpy(&(resp)->value_list[len].oid, &(req)->oid_list[index], \
           sizeof((req)->oid_list[index])); \
    memcpy(&(resp)->value_list[len].data, &err, sizeof(err)); \
    (resp)->value_list_length++; \
    continue; \
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define SNMP_GET_ERROR(resp, req, index, code, err, msg) { \
    if((req)->version == SNMP_VERSION_1) { \
      SNMP_VERSION_1_ERROR((resp), (code), (index)); } \
 \
    if((resp)->value_list_length < SNMP_MAX_NR_VALUES) { \
      SNMP_VERSION_2_ERROR((resp), (req), (index), err); } \
 \
    LOG_ERR("%s", msg); \
    return -1; \
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static const snmp_data_t snmp_no_such_object = { { '\x80', '\x00' }, 2, 2 };
static const snmp_data_t snmp_no_such_instance = { { '\x81', '\x00' }, 2, 2 };
static const snmp_data_t snmp_end_of_mib_view = { { '\x82', '\x00' }, 2, 2 };
static struct uip_udp_conn *snmp_udp_conn = NULL;
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static int
snmnp_get_handle(request_t *request, response_t *response, snmp_client_t *UNUSED(client))
{
  size_t i, pos;
  snmp_value_t *value;
  const char *msg = "Failed handling SNMP GET: value list overflow\n";

  /*
   * Search each varbinding of the request and append the value to the
   * response. Note that if the length does not match, we might have found a
   * subid of the requested one (table cell of table column)!
   */
  for(i = 0; i < request->oid_list_length; i++) {
    pos = 0;
    value = snmp_mib_find(&request->oid_list[i], &pos);
    if(!value) {
      SNMP_GET_ERROR(response, request, i, SNMP_STATUS_NO_SUCH_NAME, snmp_no_such_object, msg);
    }

    if(pos < 0) {
      SNMP_GET_ERROR(response, request, i, SNMP_STATUS_NO_SUCH_NAME, snmp_no_such_object, msg);
    }

    if(value->oid.subid_list_length == (request->oid_list[i].subid_list_length + 1)) {
      SNMP_GET_ERROR(response, request, i, SNMP_STATUS_NO_SUCH_NAME, snmp_no_such_instance, msg);
    }

    if(value->oid.subid_list_length != request->oid_list[i].subid_list_length) {
      SNMP_GET_ERROR(response, request, i, SNMP_STATUS_NO_SUCH_NAME, snmp_no_such_object, msg);
    }

    if(response->value_list_length < SNMP_MAX_NR_VALUES) {
      memcpy(&response->value_list[response->value_list_length], value, sizeof(*value));
      response->value_list_length++;
      continue;
    }

    LOG_ERR("%s", msg);
    break;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static int
snmnp_getnext_handle(request_t *request, response_t *response, snmp_client_t *UNUSED(client))
{
  size_t i;
  snmp_value_t *value;
  const char *msg = "Failed handling SNMP GETNEXT: value list overflow\n";

  /*
   * Search each varbinding of the request and append the value to the
   * response. Note that if the length does not match, we might have found a
   * subid of the requested one (table cell of table column)!
   */
  for(i = 0; i < request->oid_list_length; i++) {
    value = snmp_mib_findnext(&request->oid_list[i]);
    if(!value) {
      SNMP_GET_ERROR(response, request, i, SNMP_STATUS_NO_SUCH_NAME, snmp_end_of_mib_view, msg);
    }

    if(response->value_list_length < SNMP_MAX_NR_VALUES) {
      memcpy(&response->value_list[response->value_list_length], value, sizeof(*value));
      response->value_list_length++;
      continue;
    }

    LOG_ERR("%s", msg);
    break;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static int
snmp_set_handle(request_t *request, response_t *response, snmp_client_t *UNUSED(client))
{
  SNMP_VERSION_1_ERROR(response, (request->version == SNMP_VERSION_1)
                       ? SNMP_STATUS_NO_SUCH_NAME : SNMP_STATUS_NO_ACCESS, 0);
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static int
snmnp_getbulk_handle(request_t *request, response_t *response, snmp_client_t *UNUSED(client))
{
  size_t i, j;
  snmp_oid_t oid_list[SNMP_MAX_NR_OIDS];
  snmp_value_t *value;
  const char *msg = "Failed handling SNMP GETBULK: value list overflow\n";

  /* Make a local copy of the OID list since we are going to modify it */
  memcpy(oid_list, request->oid_list, sizeof(request->oid_list));

  /* The non-repeaters are handled like with the GETNEXT request */
  for(i = 0; i < request->oid_list_length; i++) {
    if(i >= request->non_repeaters) {
      break;
    }

    value = snmp_mib_findnext(&oid_list[i]);
    if(!value) {
      SNMP_GET_ERROR(response, request, i, SNMP_STATUS_NO_SUCH_NAME, snmp_end_of_mib_view, msg);
    }

    if(response->value_list_length < SNMP_MAX_NR_VALUES) {
      memcpy(&response->value_list[response->value_list_length], value, sizeof(*value));
      response->value_list_length++;
      continue;
    }

    LOG_ERR("%s", msg);
    break;
  }

  /*
   * The repeaters are handled like with the GETNEXT request, except that:
   *
   * - the access is interleaved (i.e. first repetition of all varbinds,
   *   then second repetition of all varbinds, then third,...)
   * - the repetitions are aborted as soon as there is no successor found
   *   for all of the varbinds
   * - other than with getnext, the last variable in the MIB is named if
   *   the variable queried is not after the end of the MIB
   */
  for(j = 0; j < request->max_repetitions; j++) {
    int found_repeater = 0;

    for(i = request->non_repeaters; i < request->oid_list_length; i++) {
      value = snmp_mib_findnext(&oid_list[i]);
      if(!value) {
        SNMP_GET_ERROR(response, request, i, SNMP_STATUS_NO_SUCH_NAME, snmp_end_of_mib_view, msg);
      }

      if(response->value_list_length < SNMP_MAX_NR_VALUES) {
        memcpy(&response->value_list[response->value_list_length], value, sizeof(*value));
        response->value_list_length++;
        memcpy(&oid_list[i], &value->oid, sizeof(value->oid));
        found_repeater++;
        continue;
      }

      LOG_ERR("%s", msg);
      break;
    }

    if(found_repeater == 0) {
      break;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static int
snmp_handler(snmp_client_t *client)
{
  static response_t snmp_response;
  static request_t snmp_request;

  /*
   * Setup request and response (other code only changes non-defaults)
   */
  memset(&snmp_request, 0, sizeof(snmp_request));
  memset(&snmp_response, 0, sizeof(snmp_response));

  /*
   * Decode the request (only checks for syntax of the packet)
   */
  if(snmp_asn1_decode_request(&snmp_request, client) == -1) {
    return -1;
  }

  /*
   * If we are using SNMP v2c or require authentication, check the community
   * string for length and validity.
   */
  if(snmp_request.version == SNMP_VERSION_2C) {
    if(strcmp(SNMP_COMMUNITY, snmp_request.community)) {
      snmp_response.error_status = (snmp_request.version == SNMP_VERSION_2C) ? SNMP_STATUS_NO_ACCESS : SNMP_STATUS_GEN_ERR;
      snmp_response.error_index = 0;

      /*
       * Encode the request (depending on error status and encode flags)
       */
      if(snmp_asn1_encode_response(&snmp_request, &snmp_response, client) == -1) {
        LOG_ERR("Error encoding request error for invalid %s community\n", snmp_request.community);
        return -1;
      }
      LOG_INFO("Request for %s is invalid\n", snmp_request.community);
      return 0;
    }
  }

  /*
   * Now handle the SNMP requests depending on their type
   */
  switch(snmp_request.type) {
  case BER_TYPE_SNMP_GET:
    if(snmnp_get_handle(&snmp_request, &snmp_response, client) == -1) {
      LOG_ERR("Error while handling get request\n");
      return -1;
    }
    break;

  case BER_TYPE_SNMP_GETNEXT:
    if(snmnp_getnext_handle(&snmp_request, &snmp_response, client) == -1) {
      LOG_ERR("Error while handling get next request\n");
      return -1;
    }
    break;

  case BER_TYPE_SNMP_SET:
    if(snmp_set_handle(&snmp_request, &snmp_response, client) == -1) {
      LOG_ERR("Error while handling set request\n");
      return -1;
    }
    break;

  case BER_TYPE_SNMP_GETBULK:
    if(snmnp_getbulk_handle(&snmp_request, &snmp_response, client) == -1) {
      LOG_ERR("Error while handling get bulk request\n");
      return -1;
    }
    break;

  default:
    LOG_ERR("UNHANDLED REQUEST TYPE %d\n", snmp_request.type);
    client->size = 0;
    return 0;
  }

  /*
   * Encode the request (depending on error status and encode flags)
   */
  if(snmp_asn1_encode_response(&snmp_request, &snmp_response, client) == -1) {
    LOG_ERR("Error while encoding response\n");
    return -1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static void
snmp_process_data(void)
{
  static snmp_client_t snmp_client;

  LOG_DBG("receiving UDP datagram from [");
  LOG_DBG_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_DBG_("]:%u", uip_ntohs(UIP_UDP_BUF->srcport));
  LOG_DBG_(" Length: %u\n", uip_datalen());

  memcpy(snmp_client.packet, uip_appdata, uip_datalen());
  snmp_client.size = uip_datalen();

  /*
   * Handle the request
   */
  if(snmp_handler(&snmp_client) == -1) {
    LOG_DBG("Error while handling the request\n");
  } else {
    LOG_DBG("Sending response\n");
    /*
     * Send the response
     */
    uip_udp_packet_sendto(snmp_udp_conn, snmp_client.packet, snmp_client.size, &UIP_IP_BUF->srcipaddr, UIP_UDP_BUF->srcport);
  }
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define SNMP_SERVER_PORT UIP_HTONS(SNMP_PORT)
PROCESS(snmp_process, "SNMP Process");
PROCESS(snmp_mib_process, "SNMP MIB Process");
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
void
snmp_init()
{
  snmp_mib_build();
  process_start(&snmp_process, NULL);
  process_start(&snmp_mib_process, NULL);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(snmp_mib_process, ev, data)
{
  static struct etimer snmp_mib_process_et;
  PROCESS_BEGIN();
  etimer_set(&snmp_mib_process_et, CLOCK_SECOND * SNMP_MIB_UPDATER_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT(); /* Same thing as PROCESS_YIELD */
    if(etimer_expired(&snmp_mib_process_et)) {
      LOG_DBG("Updating MIB\n");
      if(snmp_mib_update() == -1) {
        LOG_ERR("Error while updating MIB\n");
      }
      etimer_restart(&snmp_mib_process_et);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(snmp_process, ev, data)
{
  PROCESS_BEGIN();

  /* new connection with remote host */
  snmp_udp_conn = udp_new(NULL, 0, NULL);
  udp_bind(snmp_udp_conn, SNMP_SERVER_PORT);
  LOG_DBG("Listening on port %u\n", uip_ntohs(snmp_udp_conn->lport));

  while(1) {
    PROCESS_YIELD();

    if(ev == tcpip_event) {
      if(uip_newdata()) {
        snmp_process_data();
      }
    }
  } /* while (1) */

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

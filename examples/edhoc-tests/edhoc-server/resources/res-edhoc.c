/**
 * \file
 *      EDHOC plugtest resource [draft-selander-lake-edhoc-01] with CoAP Block-Wise Transfer [RFC7959]
 * \author
 *      Lidia Pocero <pocero@isi.gr>
 */

#include <stdio.h>
#include <string.h>
#include "coap-engine.h"
#include "coap.h"
#include "edhoc-server-API.h"

static void res_edhoc_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
RESOURCE(res_edhoc, "title=\"EDHOC resource\"", NULL, res_edhoc_post_handler, NULL, NULL);

static void
res_edhoc_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  edhoc_server_resource(request, response);
}

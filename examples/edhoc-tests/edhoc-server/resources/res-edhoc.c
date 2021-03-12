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
edhoc_server_t servidor;
static uint8_t msg_rx[MAX_DATA_LEN];
static size_t msg_rx_len;
static void res_edhoc_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
RESOURCE(res_edhoc, "title=\"EDHOC resource\"", NULL, res_edhoc_post_handler, NULL, NULL);
static size_t big_msg_len = 0;

static void
res_edhoc_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  // Example allows only one request on time. There are no checks for multiply access !!! 
  if(*offset == 0) {
    if(coap_block1_handler(request, response, msg_rx, &msg_rx_len, MAX_DATA_LEN)) {
     LOG_DBG("handeler (%d)\n", (int)msg_rx_len);
      print_buff_8_dbg(msg_rx, msg_rx_len);
      return;
    }
    else{
        LOG_DBG("RX msg (%d)\n", (int)msg_rx_len);
        print_buff_8_dbg(msg_rx, msg_rx_len);
        edhoc_server_process(request,response, &servidor,msg_rx,msg_rx_len);    
    }        
    response->payload = (uint8_t *)ctx->msg_tx;
    response->payload_len = ctx->tx_sz;
    big_msg_len = ctx->tx_sz;
    coap_set_header_block1(response, request->block1_num, 0, request->block1_size);

    if( response->payload_len  > 64) {
      coap_set_option(response, COAP_OPTION_BLOCK2);
      coap_set_header_block2(response, 0, 1, 64);
    }
    
  } else {
   
    coap_set_status_code(response, CHANGED_2_04);
    memcpy(buffer, ctx->msg_tx + *offset, 64);
    if(ctx->tx_sz - *offset < preferred_size) {
      preferred_size = ctx->tx_sz - *offset;
      *offset = -1;
    } else {
      *offset += preferred_size;
    }
    coap_set_payload(response, buffer, preferred_size);
  }
}
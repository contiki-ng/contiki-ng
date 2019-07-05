/*
 * Copyright (c) 2016, Yasuyuki Tanaka
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
 * \addtogroup sixtop
 * @{
 */
/**
 * \file
 *         6top Protocol (6P)
 * \author
 *         Yasuyuki Tanaka <yasuyuki.tanaka@inf.ethz.ch>
 */

#include "contiki-lib.h"
#include "lib/assert.h"

#include "sixtop.h"
#include "sixp-nbr.h"
#include "sixp-pkt.h"
#include "sixp-trans.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "6top"
#define LOG_LEVEL LOG_LEVEL_6TOP

static void mac_callback(void *ptr, int status, int transmissions);
static int send_back_error(sixp_pkt_type_t type, sixp_pkt_rc_t rc,
                           const sixp_pkt_t *pkt, const linkaddr_t *dest_addr);
static void handle_schedule_inconsistency(const sixtop_sf_t *sf,
                                          const sixp_pkt_t *recved_pkt,
                                          const linkaddr_t *peer_addr);
/*---------------------------------------------------------------------------*/
static void
mac_callback(void *ptr, int status, int transmissions)
{
  sixp_trans_t *trans = (sixp_trans_t *)ptr;
  sixp_trans_state_t new_state, current_state;

  assert(trans != NULL);
  if(trans == NULL) {
    LOG_ERR("6P: mac_callback() fails because trans is NULL\n");
    return;
  } else if(sixp_trans_get_state(trans) == SIXP_TRANS_STATE_WAIT_FREE) {
    /* the transaction has been invalidated; free it now */
    const sixtop_sf_t *sf = sixp_trans_get_sf(trans);
    if(sf != NULL && sf->error != NULL) {
      sf->error(SIXP_ERROR_TX_AFTER_TRANSACTION_TERMINATION,
                sixp_trans_get_cmd(trans),
                sixp_trans_get_seqno(trans),
                sixp_trans_get_peer_addr(trans));
    }
    sixp_trans_free(trans);
    return;
  }

  current_state = sixp_trans_get_state(trans);
  new_state = SIXP_TRANS_STATE_UNAVAILABLE;
  if(status == MAC_TX_OK) {
    switch(current_state) {
      case SIXP_TRANS_STATE_REQUEST_SENDING:
        new_state = SIXP_TRANS_STATE_REQUEST_SENT;
        break;
      case SIXP_TRANS_STATE_RESPONSE_SENDING:
        new_state = SIXP_TRANS_STATE_RESPONSE_SENT;
        break;
      case SIXP_TRANS_STATE_CONFIRMATION_SENDING:
        new_state = SIXP_TRANS_STATE_CONFIRMATION_SENT;
        break;
      default:
        LOG_ERR("6P: mac_callback() fails because of an unexpected state (%u)\n",
                current_state);
        /* don't change the state */
        new_state = current_state;
        break;
    }
  } else {
    /*
     * In a case of transmission failure of a request, a corresponding SF would
     * retransmit the request with a new transaction. For a response or a
     * confirmation, the same transaction will be used for retransmission as
     * long as it doesn't have timeout.
     */
    if(current_state == SIXP_TRANS_STATE_REQUEST_SENDING) {
      /* request case */
      new_state = SIXP_TRANS_STATE_TERMINATING;
    } else {
      /* response or confirmation case: stay the same state */
      new_state = current_state;
    }
  }

  if(new_state != current_state &&
     sixp_trans_transit_state(trans, new_state) != 0) {
    LOG_ERR("6P: mac_callback() fails because of state transition failure\n");
    LOG_ERR("6P: something wrong; we're terminating the trans %p\n", trans);
    (void)sixp_trans_transit_state(trans, SIXP_TRANS_STATE_TERMINATING);
    return;
  }

  sixp_trans_invoke_callback(trans,
                             status == MAC_TX_OK ?
                             SIXP_OUTPUT_STATUS_SUCCESS :
                             SIXP_OUTPUT_STATUS_FAILURE);
  sixp_trans_set_callback(trans, NULL, NULL, 0);
}
/*---------------------------------------------------------------------------*/
static int
send_back_error(sixp_pkt_type_t type, sixp_pkt_rc_t rc,
                const sixp_pkt_t *pkt, const linkaddr_t *dest_addr)
{
  sixp_trans_t *trans;

  assert(pkt != NULL);
  assert(dest_addr != NULL);

  if((rc == SIXP_PKT_RC_ERR_VERSION) ||
     (rc == SIXP_PKT_RC_ERR_SFID) ||
     (rc == SIXP_PKT_RC_ERR_BUSY) ||
     (rc == SIXP_PKT_RC_ERR_SEQNUM &&
      (trans = sixp_trans_find(dest_addr)) != NULL &&
      sixp_trans_get_state(trans) != SIXP_TRANS_STATE_REQUEST_RECEIVED)) {
    /* create a 6P packet within packetbuf */
    if(sixp_pkt_create(type, (sixp_pkt_code_t)(uint8_t)rc,
                       pkt->sfid, pkt->seqno, NULL, 0, NULL) < 0) {
      LOG_ERR("6P: failed to create a 6P packet to return an error [rc:%u]\n",
              rc);
      return -1;
    }
    /*
     * for RC_ERR_VERSION and RC_ERR_SFID, we don't care about how the
     * transmission goes for unsupported packets; no need to set
     * callback.
     *
     * for RC_ERR_BUSY, we cannot allocate another transaction. so we call
     * sixtop_output() directly
     *
     * when we're going to send RC_ERR_SEQNUM to a peer to whom we
     * have a transaction under way, we call sixtop_output() directly
     * to prevent allocating another transaction
     */
    sixtop_output(dest_addr, NULL, NULL);
  } else {
    /*
     * 6P creates a transaction to send an error other than listed
     * above. we use sixp_output to make 'nbr' have a correct
     * next_seqno.
     */
    sixp_output(type, (sixp_pkt_code_t)(uint8_t)rc, pkt->sfid,
                NULL, 0, dest_addr, NULL, NULL, 0);
  }
  LOG_ERR("6P: send an error code %u to ", rc);
  LOG_ERR_LLADDR(dest_addr);
  LOG_ERR_("\n");
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
handle_schedule_inconsistency(const sixtop_sf_t *sf,
                              const sixp_pkt_t *recved_pkt,
                              const linkaddr_t *peer_addr)
{
  assert(recved_pkt != NULL);
  assert(peer_addr != NULL);

  if(send_back_error(SIXP_PKT_TYPE_RESPONSE, SIXP_PKT_RC_ERR_SEQNUM,
                     recved_pkt, peer_addr) < 0) {
    LOG_ERR("6P: sixp_input() fails to return an error response\n");
  }
  if(sf != NULL && sf->error != NULL) {
    sf->error(SIXP_ERROR_SCHEDULE_INCONSISTENCY,
              (sixp_pkt_cmd_t)recved_pkt->code.value, recved_pkt->seqno,
              peer_addr);
  }
}
/*---------------------------------------------------------------------------*/
void
sixp_input(const uint8_t *buf, uint16_t len, const linkaddr_t *src_addr)
{
  sixp_pkt_t pkt;
  sixp_trans_t *trans;
  sixp_nbr_t *nbr;
  const sixtop_sf_t *sf;
  int16_t seqno;
  int ret;

  assert(buf != NULL && src_addr != NULL);
  if(buf == NULL || src_addr == NULL) {
    return;
  }

  if(sixp_pkt_parse(buf, len, &pkt) < 0) {
    if(pkt.version != SIXP_PKT_VERSION) {
      LOG_ERR("6P: sixp_input() unsupported version %u\n", pkt.version);
      if(send_back_error(SIXP_PKT_TYPE_RESPONSE, SIXP_PKT_RC_ERR_VERSION,
                         (const sixp_pkt_t *)&pkt, src_addr) < 0) {
        LOG_ERR("6P: sixp_input() fails to send RC_ERR_VERSION\n");
      }
    } else {
      LOG_ERR("6P: sixp_input() fails because of a malformed 6P packet\n");
    }
    return;
  }

  if(pkt.type != SIXP_PKT_TYPE_REQUEST &&
     pkt.type != SIXP_PKT_TYPE_RESPONSE &&
     pkt.type != SIXP_PKT_TYPE_CONFIRMATION) {
    LOG_ERR("6P: sixp_input() fails because of unsupported type [type:%u]\n",
            pkt.type);
    return;
  }

  if((sf = sixtop_find_sf(pkt.sfid)) == NULL) {
    LOG_ERR("6P: sixp_input() fails because SF [sfid:%u] is unavailable\n",
            pkt.sfid);
    /*
     * XXX: what if the incoming packet is a response? confirmation should be
     * sent back?
     */
    if(send_back_error(SIXP_PKT_TYPE_RESPONSE, SIXP_PKT_RC_ERR_SFID,
                       (const sixp_pkt_t *)&pkt, src_addr) < 0) {
      LOG_ERR("6P: sixp_input() fails to return an error response\n");
    };
    return;
  }

  /* Transaction Management */
  trans = sixp_trans_find(src_addr);

  if(pkt.type == SIXP_PKT_TYPE_REQUEST) {
    if(trans != NULL) {
      if(pkt.code.cmd != SIXP_PKT_CMD_CLEAR &&
         ((pkt.seqno == 0 && sixp_trans_get_seqno(trans) != 0) ||
          (pkt.seqno != 0 && sixp_trans_get_seqno(trans) == 0))) {
        /*
         * seems the peer had power-cycle; we're going to send back
         * RC_ERR_SEQNUM. in this case, we don't want to allocate
         * another transaction for this new request.
         */
        handle_schedule_inconsistency(sf, (const sixp_pkt_t *)&pkt, src_addr);
        return;
      } else {
        /* Error: not supposed to have another transaction with the peer. */
        LOG_ERR("6P: sixp_input() fails because another request [peer_addr:");
        LOG_ERR_LLADDR((const linkaddr_t *)src_addr);
        LOG_ERR_(" seqno:%u] is in process\n", sixp_trans_get_seqno(trans));
        /*
         * Although RFC 8480 says in Section 3.4.3 that we MUST send
         * RC_RESET back in this case, we use RC_ERR_BUSY
         * instead. RC_RESET requires the peer to revert SeqNum update
         * by the corresponding transaction, which may cause
         * unnecessary SeqNum disagreement. This also would make the
         * implementation complex. At this moment, no benefit is seen
         * in using RC_RESET over RC_ERR_BUSY.
         */
        if(send_back_error(SIXP_PKT_TYPE_RESPONSE, SIXP_PKT_RC_ERR_BUSY,
                           (const sixp_pkt_t *)&pkt, src_addr) < 0) {
          LOG_ERR("6P: sixp_input() fails to return an error response");
        }
        return;
      }
    }

    if((pkt.code.cmd == SIXP_PKT_CMD_CLEAR) &&
       (nbr = sixp_nbr_find(src_addr)) != NULL) {
      LOG_INFO("6P: sixp_input() reset nbr's next_seqno by CLEAR Request\n");
      sixp_nbr_reset_next_seqno(nbr);
    }

    if((trans = sixp_trans_alloc(&pkt, src_addr)) == NULL) {
      LOG_ERR("6P: sixp_input() fails because of lack of memory\n");
      if(send_back_error(SIXP_PKT_TYPE_RESPONSE, SIXP_PKT_RC_ERR_BUSY,
                         (const sixp_pkt_t *)&pkt, src_addr) < 0) {
        LOG_ERR("6P: sixp_input() fails to return an error response\n");
      }
      return;
    }

    /* Inconsistency Management */
    if(pkt.code.cmd != SIXP_PKT_CMD_CLEAR &&
       (((nbr = sixp_nbr_find(src_addr)) == NULL &&
         (pkt.seqno != 0)) ||
        ((nbr != NULL) &&
         (sixp_nbr_get_next_seqno(nbr) != 0) &&
         pkt.seqno == 0))) {
      if(trans != NULL) {
        sixp_trans_transit_state(trans,
                                 SIXP_TRANS_STATE_REQUEST_RECEIVED);
        handle_schedule_inconsistency(sf, (const sixp_pkt_t *)&pkt, src_addr);
      }
      return;
    }

  } else if(pkt.type == SIXP_PKT_TYPE_RESPONSE ||
            pkt.type == SIXP_PKT_TYPE_CONFIRMATION) {
    if(trans == NULL) {
      /* Error: should have a transaction for incoming packet */
      LOG_ERR("6P: sixp_input() fails because of no trans [peer_addr:");
      LOG_ERR_LLADDR((const linkaddr_t *)src_addr);
      LOG_ERR_("]\n");
      return;
    } else if((seqno = sixp_trans_get_seqno(trans)) < 0 ||
              seqno != pkt.seqno) {
      LOG_ERR("6P: sixp_input() fails because of invalid seqno [seqno:%u, %u]\n",
              seqno, pkt.seqno);
      /*
       * Figure 31 of RFC 8480 implies there is a chance to receive a
       * 6P Response having RC_ERR_SEQNUM and SeqNum of 0. But, it
       * shouldn't happen according to the definition of SeqNum
       * described Section 3.2.2 of RFC 8480. We discard such a 6P
       * Response silently without taking any action.
       */
      return;
    }
  }

  /* state transition */
  assert(trans != NULL);
  switch(pkt.type) {
    case SIXP_PKT_TYPE_REQUEST:
      ret = sixp_trans_transit_state(trans,
                                     SIXP_TRANS_STATE_REQUEST_RECEIVED);
      break;
    case SIXP_PKT_TYPE_RESPONSE:
      ret = sixp_trans_transit_state(trans,
                                     SIXP_TRANS_STATE_RESPONSE_RECEIVED);
      break;
    case SIXP_PKT_TYPE_CONFIRMATION:
      ret = sixp_trans_transit_state(trans,
                                     SIXP_TRANS_STATE_CONFIRMATION_RECEIVED);
      break;
    default:
      LOG_ERR("6P: sixp_input() fails because of unsupported type [type:%u]\n",
              pkt.type);
      return;
  }
  if(ret < 0) {
    LOG_ERR("6P: sixp_input() fails because of state transition failure\n");
    LOG_ERR("6P: maybe a duplicate packet to trans:%p\n", trans);
  } else if(sf->input != NULL) {
    sf->input(pkt.type, pkt.code, pkt.body, pkt.body_len, src_addr);
  } else {
  }

  return;
}
/*---------------------------------------------------------------------------*/
int
sixp_output(sixp_pkt_type_t type, sixp_pkt_code_t code, uint8_t sfid,
            const uint8_t *body, uint16_t body_len,
            const linkaddr_t *dest_addr,
            sixp_sent_callback_t func, void *arg, uint16_t arg_len)
{
  sixp_trans_t *trans;
  sixp_nbr_t *nbr;
  int16_t seqno;
  sixp_pkt_t pkt;

  assert(dest_addr != NULL);

  /* validate the state of a transaction with a specified peer */
  trans = sixp_trans_find(dest_addr);
  if(type == SIXP_PKT_TYPE_REQUEST) {
    if(trans != NULL) {
      LOG_ERR("6P: sixp_output() fails because another trans for [peer_addr:");
      LOG_ERR_LLADDR((const linkaddr_t *)dest_addr);
      LOG_ERR_("] is in process\n");
      return -1;
    } else {
      /* ready to send a request */
      /* we're going to allocate a new transaction later */
    }
  } else if(type == SIXP_PKT_TYPE_RESPONSE) {
    if(trans == NULL) {
      LOG_ERR("6P: sixp_output() fails because of no transaction [peer_addr:");
      LOG_ERR_LLADDR((const linkaddr_t *)dest_addr);
      LOG_ERR_("]\n");
      return -1;
    } else if(sixp_trans_get_state(trans) !=
              SIXP_TRANS_STATE_REQUEST_RECEIVED) {
      LOG_ERR("6P: sixp_output() fails because of invalid transaction state\n");
      return -1;
    } else {
      /* ready to send a response */
    }
  } else if(type == SIXP_PKT_TYPE_CONFIRMATION) {
    if(trans == NULL) {
      LOG_ERR("6P: sixp_output() fails because of no transaction [peer_addr:");
      LOG_ERR_LLADDR((const linkaddr_t *)dest_addr);
      LOG_ERR_("]\n");
      return -1;
    } else if(sixp_trans_get_state(trans) !=
              SIXP_TRANS_STATE_RESPONSE_RECEIVED) {
      LOG_ERR("6P: sixp_output() fails because of invalid transaction state\n");
      return -1;
    } else {
      /* ready to send a confirmation */
    }
  } else {
    LOG_ERR("6P: sixp_output() fails because of unsupported type [type:%u]\n",
            type);
    return -1;
  }

  nbr = sixp_nbr_find(dest_addr);

  /*
   * Make sure we have a nbr for the peer if the packet is a response
   * with success so that we can manage the schedule generation unless
   * the request is CLEAR or in an unsupported format, in such a case
   * we're going to return RC_ERR_VERSION or RC_ERR_SFID.
   */
  if(nbr == NULL &&
     type == SIXP_PKT_TYPE_RESPONSE &&
     sixp_trans_get_cmd(trans) != SIXP_PKT_CMD_CLEAR &&
     code.value != SIXP_PKT_RC_ERR_VERSION &&
     code.value != SIXP_PKT_RC_ERR_SFID &&
     (nbr = sixp_nbr_alloc(dest_addr)) == NULL) {
    LOG_ERR("6P: sixp_output() fails because of no memory for another nbr\n");
    return -1;
  }

  /* set SeqNum */
  if(type == SIXP_PKT_TYPE_REQUEST) {
    if(nbr == NULL &&
       (nbr = sixp_nbr_alloc(dest_addr)) == NULL) {
      LOG_ERR("6P: sixp_output() fails because it fails to allocate a nbr\n");
      return -1;
    }
    if((seqno = sixp_nbr_get_next_seqno(nbr)) < 0){
      LOG_ERR("6P: sixp_output() fails to get the next sequence number\n");
      return -1;
    }
  } else {
    assert(trans != NULL);
    if((seqno = sixp_trans_get_seqno(trans)) < 0) {
      LOG_ERR("6P: sixp_output() fails because it fails to get seqno\n");
      return -1;
    }
  }

  /* create a 6P packet within packetbuf */
  if(sixp_pkt_create(type, code, sfid,
                     (uint8_t)seqno,
                     body, body_len,
                     type == SIXP_PKT_TYPE_REQUEST ? &pkt : NULL) < 0) {
    LOG_ERR("6P: sixp_output() fails to create a 6P packet\n");
    return -1;
  }

  /* allocate a transaction for a sending request */
  if(type == SIXP_PKT_TYPE_REQUEST) {
    assert(trans == NULL);
    if((trans = sixp_trans_alloc(&pkt, dest_addr)) == NULL) {
      LOG_ERR("6P: sixp_output() is aborted because of no memory\n");
      return -1;
    } else {
      /* ready for proceed */
      LOG_DBG("6P: sixp_output() allocates trans:%p\n", trans);
    }
  }

  assert(trans != NULL);
  sixp_trans_set_callback(trans, func, arg, arg_len);
  if(sixtop_output(dest_addr, mac_callback, trans) == 0) {
    /* sixtop_output() call ends without an error */
    sixp_trans_state_t state = sixp_trans_get_state(trans);
    if(state == SIXP_TRANS_STATE_INIT) {
      /*
       * increment/update next_seqno immediately to prevent the next
       * request from having the same seqno, although RFC8480
       * describes a different way to manage seqno. If the next
       * request has the same seqno as the previous request, a delayed
       * response, which is for the previous transaction, may be
       * handled mistakenly for the next request when the previous
       * transaction is expired.
       */
      if(code.cmd == SIXP_PKT_CMD_CLEAR) {
        LOG_INFO("6P: sixp_output() reset nbr's next_seqno by CLEAR Request\n");
        sixp_nbr_reset_next_seqno(nbr);
      } else {
        sixp_nbr_increment_next_seqno(nbr);
      }
      sixp_trans_transit_state(trans, SIXP_TRANS_STATE_REQUEST_SENDING);
    } else if(state == SIXP_TRANS_STATE_REQUEST_RECEIVED) {
      sixp_trans_transit_state(trans, SIXP_TRANS_STATE_RESPONSE_SENDING);
    } else if(state == SIXP_TRANS_STATE_RESPONSE_RECEIVED) {
      sixp_trans_transit_state(trans, SIXP_TRANS_STATE_CONFIRMATION_SENDING);
    } else {
      /* shouldn't come here */
      LOG_ERR("6P: sixp_output() is called trans:%p whose state is %u\n",
              trans, state);
    }
  } else {
    if(type == SIXP_PKT_TYPE_REQUEST) {
      /* abort the transaction now */
      sixp_trans_abort(trans);
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
void
sixp_init(void)
{
  sixp_nbr_init();
  sixp_trans_init();
}
/*---------------------------------------------------------------------------*/
/** @} */

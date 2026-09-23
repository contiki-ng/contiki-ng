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
 * \file
 *         Publish-Subscribe 802.15.4 MAC commmand frames.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/cmd-broker.h"
#include "net/mac/framer/frame802154.h"
#include "lib/list.h"
#include "net/packetbuf.h"

LIST(subscriptions_list);

/*---------------------------------------------------------------------------*/
uint8_t *
cmd_broker_prepare_command(uint8_t cmd_id, const linkaddr_t *dest)
{
  /* reset packetbuf */
  packetbuf_clear();
  uint8_t *payload = packetbuf_dataptr();

  /* create frame */
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, dest);
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_CMDFRAME);
  payload[0] = cmd_id;

  return payload + 1;
}
/*---------------------------------------------------------------------------*/
void
cmd_broker_subscribe(cmd_broker_subscription_t *subscription)
{
  list_add(subscriptions_list, subscription);
}
/*---------------------------------------------------------------------------*/
void
cmd_broker_unsubscribe(cmd_broker_subscription_t *subscription)
{
  list_remove(subscriptions_list, subscription);
}
/*---------------------------------------------------------------------------*/
void
cmd_broker_publish(void)
{
  uint8_t *payload = packetbuf_dataptr();
  for(cmd_broker_subscription_t *subscription = list_head(subscriptions_list);
      subscription;
      subscription = list_item_next(subscription)) {
    if(subscription->on_command(payload[0], payload + 1)) {
      return;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
cmd_broker_init(void)
{
  list_init(subscriptions_list);
}
/*---------------------------------------------------------------------------*/

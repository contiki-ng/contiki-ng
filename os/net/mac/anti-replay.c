/*
 * Copyright (c) 2014, Hasso-Plattner-Institut.
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
 * \addtogroup llsec802154
 * @{
 *
 * \file
 *         Protects against replay attacks by comparing with the last
 *         unicast or broadcast frame counter of the sender.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/anti-replay.h"
#include "net/packetbuf.h"
#include "net/mac/framer/frame802154.h"
#include "net/mac/llsec802154.h"
#include "dev/watchdog.h"

#if LLSEC802154_USES_FRAME_COUNTER

#if ANTI_REPLAY_WITH_SUPPRESSION
uint32_t anti_replay_my_broadcast_counter;
uint32_t anti_replay_my_unicast_counter;
#else /* ANTI_REPLAY_WITH_SUPPRESSION */
/* This node's current frame counter value */
static uint32_t my_counter;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */

/*---------------------------------------------------------------------------*/
static void
order_and_set_counter(uint32_t counter)
{
  if(counter == UINT32_MAX) {
    watchdog_reboot();
  }
  frame802154_frame_counter_t reordered_counter = {
    .u32 = LLSEC802154_HTONL(counter)
  };
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1,
                     reordered_counter.u16[0]);
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3,
                     reordered_counter.u16[1]);
}
/*---------------------------------------------------------------------------*/
void
anti_replay_set_counter(struct anti_replay_info *receiver_info)
{
#if ANTI_REPLAY_WITH_SUPPRESSION
  if(packetbuf_holds_broadcast()) {
    order_and_set_counter(++anti_replay_my_broadcast_counter);
  } else {
    order_and_set_counter(++receiver_info->my_unicast_counter);
  }
#else /* ANTI_REPLAY_WITH_SUPPRESSION */
  order_and_set_counter(++my_counter);
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
}
/*---------------------------------------------------------------------------*/
uint32_t
anti_replay_get_counter(void)
{
  frame802154_frame_counter_t disordered_counter;
  disordered_counter.u16[0] =
      packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1);
  disordered_counter.u16[1] =
      packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3);
  return LLSEC802154_HTONL(disordered_counter.u32);
}
/*---------------------------------------------------------------------------*/
void
anti_replay_init_info(struct anti_replay_info *info)
{
  memset(info, 0, sizeof(struct anti_replay_info));
#if ANTI_REPLAY_WITH_SUPPRESSION
  info->my_unicast_counter = anti_replay_my_unicast_counter;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
}
/*---------------------------------------------------------------------------*/
bool
anti_replay_was_replayed(struct anti_replay_info *info)
{
  uint32_t received_counter = anti_replay_get_counter();

  if(packetbuf_holds_broadcast()) {
    /* broadcast */
    if(received_counter <= info->last_broadcast_counter) {
      return true;
    } else {
      info->last_broadcast_counter = received_counter;
      return false;
    }
  } else {
    /* unicast */
    if(received_counter <= info->last_unicast_counter) {
      return true;
    } else {
      info->last_unicast_counter = received_counter;
      return false;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
anti_replay_parse_counter(const uint8_t *p)
{
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1, p[0] | p[1] << 8);
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3, p[2] | p[3] << 8);
}
/*---------------------------------------------------------------------------*/
void
anti_replay_write_counter(uint8_t *dst)
{
  dst[0] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1) & 0xFF;
  dst[1] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1) >> 8;
  dst[2] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3) & 0xFF;
  dst[3] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3) >> 8;
}
/*---------------------------------------------------------------------------*/
uint32_t
anti_replay_read_counter(const uint8_t *src)
{
  frame802154_frame_counter_t disordered_counter;

  memcpy(disordered_counter.u8, src, 4);
  return LLSEC802154_HTONL(disordered_counter.u32);
}
/*---------------------------------------------------------------------------*/
uint8_t
anti_replay_get_counter_lsbs(void)
{
  return packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1) & 0xFF;
}
/*---------------------------------------------------------------------------*/
#if ANTI_REPLAY_WITH_SUPPRESSION
void
anti_replay_write_my_broadcast_counter(uint8_t *dst)
{
  frame802154_frame_counter_t reordered_counter = {
    reordered_counter.u32 =
        LLSEC802154_HTONL(anti_replay_my_broadcast_counter)
  };
  memcpy(dst, reordered_counter.u8, 4);
}
/*---------------------------------------------------------------------------*/
void
anti_replay_restore_counter(const struct anti_replay_info *info, uint8_t lsbs)
{
  frame802154_frame_counter_t copied_counter = {
    copied_counter.u32 = LLSEC802154_HTONL(packetbuf_holds_broadcast()
                                           ? info->last_broadcast_counter
                                           : info->last_unicast_counter)
  };

  if(lsbs < copied_counter.u8[0]) {
    copied_counter.u8[1]++;
    if(!copied_counter.u8[1]) {
      copied_counter.u8[2]++;
      if(!copied_counter.u8[2]) {
        copied_counter.u8[3]++;
      }
    }
  }
  copied_counter.u8[0] = lsbs;
  anti_replay_parse_counter(copied_counter.u8);
}
/*---------------------------------------------------------------------------*/
#else /* ANTI_REPLAY_WITH_SUPPRESSION */
void
anti_replay_set_counter_to(frame802154_frame_counter_t *counter)
{
  if(++my_counter == UINT32_MAX) {
    watchdog_reboot();
  }
  counter->u32 = my_counter;
}
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
/*---------------------------------------------------------------------------*/
#endif /* LLSEC802154_USES_FRAME_COUNTER */

/** @} */

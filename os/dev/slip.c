/* -*- C -*- */
/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "net/ipv6/uip.h"
#include "dev/slip.h"

#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335
/*---------------------------------------------------------------------------*/
PROCESS(slip_process, "SLIP driver");
/*---------------------------------------------------------------------------*/
#if SLIP_CONF_WITH_STATS
static uint16_t slip_rubbish, slip_twopackets, slip_overflow, slip_ip_drop;
#define SLIP_STATISTICS(statement) statement
#else
#define SLIP_STATISTICS(statement)
#endif
/*---------------------------------------------------------------------------*/
/* Must be at least one byte larger than UIP_BUFSIZE! */
#define RX_BUFSIZE (UIP_BUFSIZE + 16)
/*---------------------------------------------------------------------------*/
enum {
  STATE_TWOPACKETS = 0, /* We have 2 packets and drop incoming data. */
  STATE_OK = 1,
  STATE_ESC = 2,
  STATE_RUBBISH = 3,
};
/*---------------------------------------------------------------------------*/
/*
 * Variables begin and end manage the buffer space in a cyclic
 * fashion. The first used byte is at begin and end is one byte past
 * the last. I.e. [begin, end) is the actively used space.
 *
 * If begin != pkt_end we have a packet at [begin, pkt_end),
 * furthermore, if state == STATE_TWOPACKETS we have one more packet at
 * [pkt_end, end). If more bytes arrive in state STATE_TWOPACKETS
 * they are discarded.
 */
static uint8_t state = STATE_TWOPACKETS;
static uint16_t begin, next_free;
static uint8_t rxbuf[RX_BUFSIZE];
static uint16_t pkt_end;    /* SLIP_END tracker. */

static void (*input_callback)(void) = NULL;
/*---------------------------------------------------------------------------*/
void
slip_set_input_callback(void (*c)(void))
{
  input_callback = c;
}
/*---------------------------------------------------------------------------*/
void
slip_send(void)
{
  slip_write(uip_buf, uip_len);
}
/*---------------------------------------------------------------------------*/
void
slip_write(const void *_ptr, int len)
{
  const uint8_t *ptr = _ptr;
  uint16_t i;
  uint8_t c;

  slip_arch_writeb(SLIP_END);

  for(i = 0; i < len; ++i) {
    c = *ptr++;
    if(c == SLIP_END) {
      slip_arch_writeb(SLIP_ESC);
      c = SLIP_ESC_END;
    } else if(c == SLIP_ESC) {
      slip_arch_writeb(SLIP_ESC);
      c = SLIP_ESC_ESC;
    }
    slip_arch_writeb(c);
  }

  slip_arch_writeb(SLIP_END);
}
/*---------------------------------------------------------------------------*/
static void
rxbuf_init(void)
{
  begin = next_free = pkt_end = 0;
  state = STATE_OK;
}
/*---------------------------------------------------------------------------*/
static uint16_t
slip_poll_handler(uint8_t *outbuf, uint16_t blen)
{
  /*
   * Interrupt can not change begin but may change pkt_end.
   * If pkt_end != begin it will not change again.
   */
  if(begin != pkt_end) {
    uint16_t len;
    uint16_t cur_next_free;
    uint16_t cur_ptr;
    int esc = 0;

    if(begin < pkt_end) {
      uint16_t i;
      len = 0;
      for(i = begin; i < pkt_end; ++i) {
        if(len > blen) {
          len = 0;
          break;
        }
        if(esc) {
          if(rxbuf[i] == SLIP_ESC_ESC) {
            outbuf[len] = SLIP_ESC;
            len++;
          } else if(rxbuf[i] == SLIP_ESC_END) {
            outbuf[len] = SLIP_END;
            len++;
          }
          esc = 0;
        } else if(rxbuf[i] == SLIP_ESC) {
          esc = 1;
        } else {
          outbuf[len] = rxbuf[i];
          len++;
        }
      }
    } else {
      uint16_t i;
      len = 0;
      for(i = begin; i < RX_BUFSIZE; ++i) {
        if(len > blen) {
          len = 0;
          break;
        }
        if(esc) {
          if(rxbuf[i] == SLIP_ESC_ESC) {
            outbuf[len] = SLIP_ESC;
            len++;
          } else if(rxbuf[i] == SLIP_ESC_END) {
            outbuf[len] = SLIP_END;
            len++;
          }
          esc = 0;
        } else if(rxbuf[i] == SLIP_ESC) {
          esc = 1;
        } else {
          outbuf[len] = rxbuf[i];
          len++;
        }
      }
      for(i = 0; i < pkt_end; ++i) {
        if(len > blen) {
          len = 0;
          break;
        }
        if(esc) {
          if(rxbuf[i] == SLIP_ESC_ESC) {
            outbuf[len] = SLIP_ESC;
            len++;
          } else if(rxbuf[i] == SLIP_ESC_END) {
            outbuf[len] = SLIP_END;
            len++;
          }
          esc = 0;
        } else if(rxbuf[i] == SLIP_ESC) {
          esc = 1;
        } else {
          outbuf[len] = rxbuf[i];
          len++;
        }
      }
    }

    /* Remove data from buffer together with the copied packet. */
    pkt_end = pkt_end + 1;
    if(pkt_end == RX_BUFSIZE) {
      pkt_end = 0;
    }
    if(pkt_end != next_free) {
      cur_next_free = next_free;
      cur_ptr = pkt_end;
      while(cur_ptr != cur_next_free) {
        if(rxbuf[cur_ptr] == SLIP_END) {
          uint16_t tmp_begin = pkt_end;
          pkt_end = cur_ptr;
          begin = tmp_begin;
          /* One more packet is buffered, need to be polled again! */
          process_poll(&slip_process);
          break;
        }
        cur_ptr++;
        if(cur_ptr == RX_BUFSIZE) {
          cur_ptr = 0;
        }
      }
      if(cur_ptr == cur_next_free) {
        /* no more pending full packet found */
        begin = pkt_end;
      }
    } else {
      begin = pkt_end;
    }
    return len;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(slip_process, ev, data)
{
  PROCESS_BEGIN();

  rxbuf_init();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    /* Move packet from rxbuf to buffer provided by uIP. */
    uip_len = slip_poll_handler(uip_buf, UIP_BUFSIZE);

    if(uip_len > 0) {
      if(input_callback) {
        input_callback();
      }
      tcpip_input();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
int
slip_input_byte(unsigned char c)
{
  uint16_t cur_end;
  switch(state) {
  case STATE_RUBBISH:
    if(c == SLIP_END) {
      state = STATE_OK;
    }
    return 0;

  case STATE_ESC:
    if(c != SLIP_ESC_END && c != SLIP_ESC_ESC) {
      state = STATE_RUBBISH;
      SLIP_STATISTICS(slip_rubbish++);
      next_free = pkt_end;    /* remove rubbish */
      return 0;
    }
    state = STATE_OK;
    break;
  }

  if(c == SLIP_ESC) {
    state = STATE_ESC;
  }

  /* add_char: */
  cur_end = next_free;
  next_free = next_free + 1;
  if(next_free == RX_BUFSIZE) {
    next_free = 0;
  }
  if(next_free == begin) {         /* rxbuf is full */
    state = STATE_RUBBISH;
    SLIP_STATISTICS(slip_overflow++);
    next_free = pkt_end;            /* remove rubbish */
    return 0;
  }
  rxbuf[cur_end] = c;

  if(c == SLIP_END) {
    /*
     * We have a new packet, possibly of zero length.
     *
     * There may already be one packet buffered.
     */
    if(cur_end != pkt_end) {  /* Non zero length. */
      if(begin == pkt_end) {  /* None buffered. */
        pkt_end = cur_end;
      } else {
        SLIP_STATISTICS(slip_twopackets++);
      }
      process_poll(&slip_process);
      return 1;
    } else {
      /* Empty packet, reset the pointer */
      next_free = cur_end;
    }
    return 0;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/

/*
 * Copyright (c) 2017, RISE SICS AB.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
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
 *         DTLS support for CoAP
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "dtls-support.h"
#include "lib/random.h"
#include <stdio.h>
#include <stdarg.h>

#define DEBUG DEBUG_FULLY
#include "dtls_debug.h"

static dtls_context_t the_dtls_context;
static dtls_cipher_context_t cipher_context;
static uint8_t lock_context = 0;
/*---------------------------------------------------------------------------*/
dtls_context_t *
dtls_context_acquire(void)
{
  if(lock_context) {
    return NULL;
  }
  lock_context = 1;
  PRINTF("DS: Allocated context\n");
  return &the_dtls_context;
}
/*---------------------------------------------------------------------------*/
void
dtls_context_release(dtls_context_t *context)
{
  if(context == &the_dtls_context) {
    lock_context = 0;
  }
}
/*---------------------------------------------------------------------------*/
dtls_cipher_context_t *
dtls_cipher_context_acquire(void)
{
  return &cipher_context;
}
/*---------------------------------------------------------------------------*/
void
dtls_cipher_context_release(dtls_cipher_context_t *c)
{
}
/*---------------------------------------------------------------------------*/
void
dtls_ticks(dtls_tick_t *t)
{
  *t = clock_time();
}
/*---------------------------------------------------------------------------*/
int
dtls_fill_random(uint8_t *buf, size_t len)
{
  int i;
  if(buf) {
    for(i = 0; i < len; i++) {
      buf[i] = random_rand() & 0xff;
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* message retransmission */
/*---------------------------------------------------------------------------*/
static void
dtls_retransmit_callback(void *ptr)
{
  dtls_context_t *ctx;
  clock_time_t now;
  clock_time_t next;

  ctx = ptr;
  now = clock_time();
  /* Just one retransmission per timer scheduling */
  dtls_check_retransmit(ctx, &next, 0);

  /* need to set timer to some value even if no nextpdu is available */
  if(next != 0) {
    ctimer_set(&ctx->support.retransmit_timer,
               next <= now ? 1 : next - now,
               dtls_retransmit_callback, ctx);
  }
}
/*---------------------------------------------------------------------------*/
void
dtls_set_retransmit_timer(dtls_context_t *ctx, unsigned int timeout)
{
  ctimer_set(&ctx->support.retransmit_timer, timeout,
             dtls_retransmit_callback, ctx);
}
/*---------------------------------------------------------------------------*/
void
dtls_session_init(session_t *sess)
{
  memset(sess, 0, sizeof(session_t));
}
/*---------------------------------------------------------------------------*/
int
dtls_session_equals(const session_t *a, const session_t *b)
{
  coap_endpoint_t *e1 = (coap_endpoint_t *)a;
  coap_endpoint_t *e2 = (coap_endpoint_t *)b;

#if DEBUG
  printf(" **** EP:");
  coap_endpoint_print(e1);
  printf(" =?= ");
  coap_endpoint_print(e2);
  printf(" => %d\n", coap_endpoint_cmp(e1, e2));
#endif /* DEBUG */

  return coap_endpoint_cmp(e1, e2);
}
/*---------------------------------------------------------------------------*/
void *
dtls_session_get_address(const session_t *a)
{
  /* improve this to only contain the addressing info */
  return (void *)a;
}
/*---------------------------------------------------------------------------*/
int dtls_session_get_address_size(const session_t *a)
{
  /* improve this to only contain the addressing info */
  return sizeof(session_t);
}
/*---------------------------------------------------------------------------*/
void
dtls_session_print(const session_t *a)
{
  coap_endpoint_print((const coap_endpoint_t *)a);
}
/*---------------------------------------------------------------------------*/
size_t
dsrv_print_addr(const session_t *addr, char *buf, size_t len)
{
  if(len > 1) {
    /* TODO print endpoint */
    buf[0] = '[';
    buf[1] = ']';
    return 2;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
extern char *loglevels[];
static inline size_t
print_timestamp(log_t level)
{
  dtls_tick_t now;
  dtls_ticks(&now);
  if(level <= DTLS_LOG_DEBUG) {
    printf("%s ", loglevels[level]);
  }
  return printf("%5lu ", (unsigned long)now);
}
/*---------------------------------------------------------------------------*/
#ifdef HAVE_VPRINTF
void
dsrv_log(log_t level, char *format, ...)
{
  va_list ap;

  if(dtls_get_log_level() < level) {
    return;
  }

  print_timestamp(level);

  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
}

#endif /* HAVE_VPRINTF */
/*---------------------------------------------------------------------------*/
void
dtls_dsrv_hexdump_log(log_t level, const char *name, const unsigned char *buf, size_t length, int extend)
{
  int n = 0;

  if(dtls_get_log_level() < level) {
    return;
  }

  print_timestamp(level);

  if(extend) {
    printf("%s: (%zu bytes):\n", name, length);

    while(length--) {
      if(n % 16 == 0) {
        printf("%08X ", n);
      }
      printf("%02X ", *buf++);

      n++;
      if(n % 8 == 0) {
	if(n % 16 == 0) {
	  printf("\n");
	} else {
	  printf(" ");
        }
      }
    }
  } else {
    printf("%s: (%zu bytes): ", name, length);
    while(length--) {
      printf("%02X", *buf++);
    }
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
void
dtls_support_init(void)
{
  dtls_info("dtls_support_init.\n");
}
/*---------------------------------------------------------------------------*/

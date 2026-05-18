/*
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-os OS drivers
 * @{
 *
 * \addtogroup nrf-dbg Debug driver
 * @{
 *
 * \file
 *         Debug driver for the nRF.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "lib/dbg-io/dbg.h"
#include "uarte-arch.h"
#include "usb.h"
/*---------------------------------------------------------------------------*/
#if PLATFORM_DBG_CONF_USB
static inline void
write_byte(uint8_t b)
{
  usb_write(&b, sizeof(b));
}
#define flush() usb_flush()
#else /* PLATFORM_DBG_CONF_USB */
#define write_byte(b) uarte_write(b)
#define flush()
#endif /* PLATFORM_DBG_CONF_USB */
/*---------------------------------------------------------------------------*/
#define ANSI_ESCAPE "\033["
#define ANSI_RESET  ANSI_ESCAPE "0m"
#define ANSI_CYAN   ANSI_ESCAPE "36m"
#define ANSI_GREEN  ANSI_ESCAPE "32m"
/*---------------------------------------------------------------------------*/
static void
write_string_raw(const char *s)
{
  while(s != NULL && *s != '\0') {
    write_byte(*s++);
  }
}
/*---------------------------------------------------------------------------*/
#if TRUSTZONE_NONSECURE
#include "trustzone/tz-api.h"
#include "sys/ctimer.h"

#define DBG_BUF_SIZE 256
static char dbg_buf[DBG_BUF_SIZE];
static uint16_t dbg_pos;
static struct ctimer dbg_flush_timer;
/*---------------------------------------------------------------------------*/
static void
dbg_flush(void)
{
  if(dbg_pos > 0) {
    tz_api_print(dbg_buf, dbg_pos);
    dbg_pos = 0;
  }
}
/*---------------------------------------------------------------------------*/
static void
dbg_flush_callback(void *ptr)
{
  dbg_flush();
}
/*---------------------------------------------------------------------------*/
dbg_output_context_t
dbg_output_context_swap(dbg_output_context_t ctx)
{
  (void)ctx;
  return DBG_OUTPUT_CONTEXT_DEFAULT;
}
/*---------------------------------------------------------------------------*/
int
dbg_putchar(int c)
{
  if(dbg_pos < DBG_BUF_SIZE - 1) {
    dbg_buf[dbg_pos++] = c;
  }

  if(c == '\n' || dbg_pos >= DBG_BUF_SIZE - 1) {
    ctimer_stop(&dbg_flush_timer);
    dbg_flush();
  } else if(dbg_pos == 1) {
    /* First char in buffer — start a short flush timer */
    ctimer_set(&dbg_flush_timer, CLOCK_SECOND / 64, dbg_flush_callback, NULL);
  }

  return c;
}
#else
static bool line_start = true;
static dbg_output_context_t output_context = DBG_OUTPUT_CONTEXT_DEFAULT;
/*---------------------------------------------------------------------------*/
static const char *
context_color(dbg_output_context_t ctx)
{
#if defined(TRUSTZONE_SECURE)
  switch(ctx) {
  case DBG_OUTPUT_CONTEXT_NONSECURE:
    return ANSI_GREEN;
  case DBG_OUTPUT_CONTEXT_DEFAULT:
  default:
    return ANSI_CYAN;
  }
#else
  (void)ctx;
  return NULL;
#endif
}
/*---------------------------------------------------------------------------*/
dbg_output_context_t
dbg_output_context_swap(dbg_output_context_t ctx)
{
  dbg_output_context_t previous = output_context;
  output_context = ctx;
  return previous;
}
/*---------------------------------------------------------------------------*/
int
dbg_putchar(int c)
{
  const char *color = context_color(output_context);

  if(line_start && color != NULL && c != '\n') {
    write_string_raw(color);
    line_start = false;
  }

  if(c == '\n') {
    if(color != NULL) {
      write_string_raw(ANSI_RESET);
    }
    write_byte('\r');
    line_start = true;
  }
  write_byte(c);

  if(c == '\n') {
    flush();
  } else {
    line_start = false;
  }

  return c;
}
#endif /* TRUSTZONE_NONSECURE */
/*---------------------------------------------------------------------------*/
unsigned int
dbg_send_bytes(const unsigned char *s, unsigned int len)
{
  unsigned int i;

  if(s == NULL) {
    return 0;
  }

  for(i = 0; i < len; i++) {
    dbg_putchar(s[i]);
  }

  flush();

  return i;
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */

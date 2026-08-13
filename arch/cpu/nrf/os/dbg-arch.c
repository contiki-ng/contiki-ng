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

#include "uarte-arch.h"
#include "usb.h"
/*---------------------------------------------------------------------------*/
#if PLATFORM_DBG_CONF_USB
#define write_byte(b) usb_write((uint8_t *)&b, sizeof(uint8_t))
#define flush()       usb_flush()
#else /* PLATFORM_DBG_CONF_USB */
#define write_byte(b) uarte_write(b)
#define flush()
#endif /* PLATFORM_DBG_CONF_USB */
/*---------------------------------------------------------------------------*/
#if defined(NRF5340_XXAA_NETWORK)
/*
 * On the nRF5340 network core, redirect all debug output to a shared
 * memory ring buffer. The application core drains it and prints with
 * a [NET] prefix, avoiding UART pin contention between the two cores.
 */
#include "nrf-ipc.h"
/*---------------------------------------------------------------------------*/
int
dbg_putchar(int c)
{
  volatile struct nrf_ipc_shared_mem *shm = NRF_IPC_SHARED_MEM;
  uint16_t head = shm->log.head;
  uint16_t next = (head + 1) % NRF_IPC_LOG_BUF_SIZE;

  /* Drop the character if the buffer is full. */
  if(next == shm->log.tail) {
    shm->log.overflow++;
    return c;
  }

  shm->log.data[head] = (char)c;
  __DMB();
  shm->log.head = next;

  return c;
}
/*---------------------------------------------------------------------------*/
#elif NRF_TRUSTZONE_NONSECURE
#include "trustzone/tz-api.h"

#define DBG_BUF_SIZE 256
static char dbg_buf[DBG_BUF_SIZE];
static uint16_t dbg_pos;
/*---------------------------------------------------------------------------*/
int
dbg_putchar(int c)
{
  if(dbg_pos < DBG_BUF_SIZE - 1) {
    dbg_buf[dbg_pos++] = c;
  }

  if(c == '\n' || dbg_pos >= DBG_BUF_SIZE - 1) {
    /* Strip the trailing newline; tz_api_println adds one. */
    uint16_t len = (dbg_pos > 0 && dbg_buf[dbg_pos - 1] == '\n')
                   ? dbg_pos - 1 : dbg_pos;
    dbg_buf[len] = '\0';
    tz_api_println(dbg_buf, len);
    dbg_pos = 0;
  }

  return c;
}
#else
int
dbg_putchar(int c)
{
  write_byte(c);

  if(c == '\n') {
    flush();
  }

  return c;
}
#endif /* NRF_TRUSTZONE_NONSECURE */
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

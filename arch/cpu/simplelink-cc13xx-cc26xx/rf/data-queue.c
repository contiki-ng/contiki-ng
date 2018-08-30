/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
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
 * \addtogroup cc13xx-cc26xx-rf-data-queue
 * @{
 *
 * \file
 *        Implementation of the CC13xx/CC26xx RF data queue.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/cc.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_data_entry.h)
/*---------------------------------------------------------------------------*/
#include "rf/data-queue.h"
/*---------------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* RX buf configuration */
#define RX_BUF_CNT        RF_CONF_RX_BUF_CNT
#define RX_BUF_SIZE       RF_CONF_RX_BUF_SIZE
/*---------------------------------------------------------------------------*/
/* Receive buffer entries with room for 1 IEEE 802.15.4 frame in each */
typedef union {
  data_entry_t data_entry;
  uint8_t buf[RX_BUF_SIZE];
} rx_buf_t CC_ALIGN (4);
/*---------------------------------------------------------------------------*/
typedef struct {
  /* RX bufs */
  rx_buf_t bufs[RX_BUF_CNT];
  /* RFC data queue object */
  data_queue_t data_queue;
  /* Current data entry in use by RF */
  data_entry_t *curr_entry;
  /* Size in bytes of length field in data entry */
  size_t lensz;
} rx_data_queue_t;

static rx_data_queue_t rx_data_queue;
/*---------------------------------------------------------------------------*/
static void
rx_bufs_init(void)
{
  data_entry_t *data_entry;
  size_t i;

  for(i = 0; i < RX_BUF_CNT; ++i) {
    data_entry = &(rx_data_queue.bufs[i].data_entry);

    data_entry->status = DATA_ENTRY_PENDING;
    data_entry->config.type = DATA_ENTRY_TYPE_GEN;
    data_entry->config.lenSz = rx_data_queue.lensz;
    data_entry->length = RX_BUF_SIZE - sizeof(data_entry_t);
    /* Point to fist entry if this is last entry, else point to next entry */
    data_entry->pNextEntry = ((i + 1) == RX_BUF_CNT)
      ? rx_data_queue.bufs[0].buf
      : rx_data_queue.bufs[i + 1].buf;
  }
}
/*---------------------------------------------------------------------------*/
static void
rx_bufs_reset(void)
{
  size_t i;
  for(i = 0; i < RX_BUF_CNT; ++i) {
    data_entry_t *const data_entry = &(rx_data_queue.bufs[i].data_entry);

    /* Clear length bytes */
    memset(&(data_entry->data), 0x0, rx_data_queue.lensz);
    /* Set status to Pending */
    data_entry->status = DATA_ENTRY_PENDING;
  }
}
/*---------------------------------------------------------------------------*/
data_queue_t *
data_queue_init(size_t lensz)
{
  rx_data_queue.lensz = lensz;

  /* Initialize RX buffers */
  rx_bufs_init();

  /* Configure data queue as circular buffer */
  rx_data_queue.data_queue.pCurrEntry = rx_data_queue.bufs[0].buf;
  rx_data_queue.data_queue.pLastEntry = NULL;

  /* Set current read pointer to first element */
  rx_data_queue.curr_entry = &(rx_data_queue.bufs[0].data_entry);

  return &rx_data_queue.data_queue;
}
/*---------------------------------------------------------------------------*/
void
data_queue_reset(void)
{
  rx_bufs_reset();

  /* Only need to reconfigure pCurrEntry */
  rx_data_queue.data_queue.pCurrEntry = rx_data_queue.bufs[0].buf;

  /* Set current read pointer to first element */
  rx_data_queue.curr_entry = &(rx_data_queue.bufs[0].data_entry);
}
/*---------------------------------------------------------------------------*/
data_entry_t *
data_queue_current_entry(void)
{
  return rx_data_queue.curr_entry;
}
/*---------------------------------------------------------------------------*/
void
data_queue_release_entry(void)
{
  data_entry_t *const curr_entry = rx_data_queue.curr_entry;
  uint8_t *const frame_ptr = (uint8_t *)&(curr_entry->data);

  /* Clear length bytes */
  memset(frame_ptr, 0x0, rx_data_queue.lensz);
  /* Set status to Pending */
  curr_entry->status = DATA_ENTRY_PENDING;

  /* Move current entry to the next entry */
  rx_data_queue.curr_entry = (data_entry_t *)(curr_entry->pNextEntry);
}
/*---------------------------------------------------------------------------*/
/** @} */

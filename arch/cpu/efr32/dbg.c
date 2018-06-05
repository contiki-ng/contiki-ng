/*
 *  Copyright (c) 2017, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements the OpenThread platform abstraction for UART communication.
 *
 */

#include "contiki.h"
#include <stddef.h>
#include "em_core.h"
#include "uartdrv.h"
#include <string.h>

#define USART_INIT                                                      \
  {                                                                     \
      USART0,                                   /* USART port */        \
      115200,                                 /* Baud rate */           \
      BSP_SERIAL_APP_TX_LOC,          /* USART Tx pin location number */ \
      BSP_SERIAL_APP_RX_LOC,          /* USART Rx pin location number */ \
      (USART_Stopbits_TypeDef)USART_FRAME_STOPBITS_ONE, /* Stop bits */ \
      (USART_Parity_TypeDef)USART_FRAME_PARITY_NONE,    /* Parity */    \
      (USART_OVS_TypeDef)USART_CTRL_OVS_X16,            /* Oversampling mode*/ \
      false,                                            /* Majority vote disable */ \
      uartdrvFlowControlNone,                         /* Flow control */ \
      BSP_SERIAL_APP_CTS_PORT,                          /* CTS port number */ \
      BSP_SERIAL_APP_CTS_PIN,                           /* CTS pin number */ \
      BSP_SERIAL_APP_RTS_PORT,                          /* RTS port number */ \
      BSP_SERIAL_APP_RTS_PIN,                           /* RTS pin number */ \
      (UARTDRV_Buffer_FifoQueue_t *)&sUartRxQueue,      /* RX operation queue */ \
      (UARTDRV_Buffer_FifoQueue_t *)&sUartTxQueue,      /* TX operation queue */ \
      BSP_SERIAL_APP_CTS_LOC,                           /* CTS location */ \
      BSP_SERIAL_APP_RTS_LOC                            /* RTS location */ \
}

DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_RX_BUFS, sUartRxQueue);
DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_TX_BUFS, sUartTxQueue);

#define kReceiveFifoSize 128

static UARTDRV_HandleData_t sUartHandleData;
static UARTDRV_Handle_t     sUartHandle = &sUartHandleData;
static uint8_t              sReceiveBuffer[2];
static volatile uint8_t     txbuzy = 0;
static int (* input_handler)(unsigned char c);

typedef struct ReceiveFifo_t
{
  // The data buffer
  uint8_t mBuffer[kReceiveFifoSize];
  // The offset of the first item written to the list.
  uint16_t mHead;
  // The offset of the next item to be written to the list.
  uint16_t mTail;
} ReceiveFifo_t;


static ReceiveFifo_t sReceiveFifo;

#define TX_SIZE 1024
static uint8_t tx_buf[TX_SIZE];
/* first char to read */
static uint16_t rpos = 0;
/* last char written (or next to write) */
static uint16_t wpos = 0;

/* The process for receiving packets */
PROCESS(serial_proc, "efr32 serial driver");

static void
receiveDone(UARTDRV_Handle_t aHandle, Ecode_t aStatus, uint8_t *aData, UARTDRV_Count_t aCount)
{
  /* We can only write if incrementing mTail doesn't equal mHead */
  if(sReceiveFifo.mHead != (sReceiveFifo.mTail + 1) % kReceiveFifoSize) {
    sReceiveFifo.mBuffer[sReceiveFifo.mTail] = aData[0];
    sReceiveFifo.mTail = (sReceiveFifo.mTail + 1) % kReceiveFifoSize;
  }
  UARTDRV_Receive(aHandle, aData, 1, receiveDone);
  process_poll(&serial_proc);
}

static void
transmitDone(UARTDRV_Handle_t aHandle, Ecode_t aStatus, uint8_t *aData, UARTDRV_Count_t aCount)
{
  (void) aHandle;
  (void) aStatus;
  (void) aData;
  (void) aCount;
  txbuzy = 0;
  process_poll(&serial_proc);
}

static void
process_receive(void)
{
  if(input_handler == NULL) {
    return;
  }
  // Copy tail to prevent multiple reads
  uint16_t tail = sReceiveFifo.mTail;
  int i;
  // If the data wraps around, process the first part
  if(sReceiveFifo.mHead > tail) {
    for(i = sReceiveFifo.mHead; i < kReceiveFifoSize; i++) {
      input_handler(sReceiveFifo.mBuffer[i]);
    }
    /* Reset the buffer mHead back to zero. */
    sReceiveFifo.mHead = 0;
  }

  // For any data remaining, process it
  if(sReceiveFifo.mHead != tail) {
    for(i = sReceiveFifo.mHead; i < tail; i++) {
      input_handler(sReceiveFifo.mBuffer[i]);
      /* Set mHead to the local tail we have cached */
      sReceiveFifo.mHead = tail;
    }
  }
}


void dbg_init(void)
{
  UARTDRV_Init_t uartInit = USART_INIT;
  sReceiveFifo.mHead = 0;
  sReceiveFifo.mTail = 0;

  UARTDRV_Init(sUartHandle, &uartInit);

  process_start(&serial_proc, NULL);

  for(uint8_t i = 0; i < sizeof(sReceiveBuffer); i++) {
    UARTDRV_Receive(sUartHandle, &sReceiveBuffer[i],
                    sizeof(sReceiveBuffer[i]), receiveDone);
  }
}


/*---------------------------------------------------------------------------*/
static unsigned char buf[64];

static void
process_transmit(void)
{
  /* send max 64 per transmit */
  int len = 0;

  if(txbuzy) {
    process_poll(&serial_proc);
    return;
  }

  if(wpos == rpos) {
    /* nothing to read */
    return;
  }

  /* did we wrap? */
  if(wpos < rpos) {
    len = TX_SIZE - rpos;
  } else {
    /* wpos > rpos */
    len = wpos - rpos;
  }
  if(len > 64) {
    len = 64;
  }
  memcpy(buf, &tx_buf[rpos], len);
  rpos = (rpos + len) % TX_SIZE;

  /* transmission ongoing... */
  txbuzy = 1;
  UARTDRV_Transmit(sUartHandle, (uint8_t *)buf, len, transmitDone);
}

/*---------------------------------------------------------------------------*/
unsigned int
dbg_send_bytes(const unsigned char *seq, unsigned int len)
{
  /* how should we handle this to not get trashed data... */
  int i;
  /* This will drop bytes if too many printed too soon */
  for(i = 0; i < len; i++) {
    tx_buf[wpos] = seq[i];
    wpos = (wpos + 1) % TX_SIZE;
  }
  process_transmit();
  process_poll(&serial_proc);
  return len;
}

/*---------------------------------------------------------------------------*/
void
dbg_putchar(const char ch)
{
  dbg_send_bytes((uint8_t *)&ch, 1);
}
/*---------------------------------------------------------------------------*/
void
dbg_set_input_handler(int (* handler)(unsigned char c))
{
  input_handler = handler;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(serial_proc, ev, data)
{
  PROCESS_BEGIN();
  while(1) {
    PROCESS_WAIT_EVENT();
    process_receive();
    process_transmit();
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

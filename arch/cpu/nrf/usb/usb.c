/*
 * Copyright (C) 2021 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 * \addtogroup nrf-usb USB driver
 * @{
 * *
 * \file
 *         USB driver for the nRF.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "usb.h"
#include "usb_descriptors.h"

#include "tusb.h"
#include "usbd.h"
/*---------------------------------------------------------------------------*/
#define BULK_PACKET_SIZE 64
/*---------------------------------------------------------------------------*/
static int (*input_handler)(unsigned char c) = NULL;
static unsigned char usb_buffer[BULK_PACKET_SIZE];
static char manufacturer[] = "Contiki-NG";
static char product[] = "Contiki-NG USB";
static char cdc_interface[] = "Contiki-NG CDC";
/*---------------------------------------------------------------------------*/
PROCESS(usb_arch_process, "USB Arch");
/*---------------------------------------------------------------------------*/
void
usb_interrupt_handler(void)
{
  tud_int_handler(0);

  /* Poll the process since we are in interrupt context */
  process_poll(&usb_arch_process);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
void
usb_set_input(int (*input)(unsigned char c))
{
  input_handler = input;
}
/*---------------------------------------------------------------------------*/
void
usb_init(void)
{
  usb_descriptor_set_manufacturer(manufacturer);
  usb_descriptor_set_product(product);
  usb_descriptor_set_cdc_interface(cdc_interface);

  usb_arch_init();

  /* Initialize the usb process */
  process_start(&usb_arch_process, NULL);
}
/*---------------------------------------------------------------------------*/
void
usb_write(uint8_t *buffer, uint32_t buffer_size)
{
  uint32_t i;

  i = 0;
  /* Operating with BULK_PACKET_SIZE as 64 (Not high speed) */
  while(buffer_size > BULK_PACKET_SIZE) {
    tud_cdc_write(buffer + i, BULK_PACKET_SIZE);
    usb_flush();
    i += BULK_PACKET_SIZE;
    buffer_size -= BULK_PACKET_SIZE;
  }

  /* Don't flush on the last write */
  tud_cdc_write(buffer + i, buffer_size);
}
/*---------------------------------------------------------------------------*/
void
usb_flush(void)
{
  tud_cdc_write_flush();
  /* Call the task to handle the events immediately */
  tud_task();
}
/*---------------------------------------------------------------------------*/
void
cdc_task(void)
{
  uint32_t usb_read_length;
  uint32_t i;

  if(tud_cdc_available()) {
    usb_read_length = tud_cdc_read(usb_buffer, sizeof(usb_buffer));
    for(i = 0; i < usb_read_length && input_handler; i++) {
      input_handler(usb_buffer[i]);
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(usb_arch_process, ev, data)
{
  PROCESS_BEGIN();

  tusb_init();

  while(1) {
    PROCESS_YIELD_UNTIL(tud_task_event_ready());

    tud_task();

    cdc_task();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

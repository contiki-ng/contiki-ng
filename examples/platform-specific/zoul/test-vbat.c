/*
 * Copyright (c) 2016, Zolertia - http://www.zolertia.com
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
 */
/*---------------------------------------------------------------------------*/
/**
 * \file
 *         An example showing how to get VBAT Voltage functionality on
 *         RE-Mote Platform
 * \author
 *         Erik Bellido <ebellido@zolertia.com>
 *         Aitor Mejias <amejias@zolertia.com>
 */
/*---------------------------------------------------------------------------*/
/* This is the main contiki header, it should be included always */
#include "contiki.h"
#include "dev/i2c.h"
#include "sys/etimer.h"

#if CONTIKI_BOARD_REMOTE_REVB
#include "power-mgmt.h"
#endif
/*---------------------------------------------------------------------------*/

/*#define freq I2C_SCL_NORMAL_BUS_SPEED */

/* RE-Mote revision B, low-power PIC version */
#define PM_EXPECTED_VERSION               0x20

/*---------------------------------------------------------------------------*/
/* We are going to create three different processes, with its own printable
 * name.  Processes are a great way to run different applications and features
 * in parallel
 */
PROCESS(test_VBAT_process, "Test VBAT process");

/* But we are only going to automatically start the first two */
AUTOSTART_PROCESSES(&test_VBAT_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_VBAT_process, ev, data)
{
  PROCESS_BEGIN();

  /* only supported on Revision B */
#if CONTIKI_BOARD_REMOTE_REVB

  static struct etimer et;
  static uint8_t aux = 0x00;
  static uint16_t voltage = 0x00;

  if(pm_enable() != PM_SUCCESS) {
    printf("PM Failed \n");
  } else if(pm_enable() == PM_SUCCESS) {
    printf("Process PM started\n");
  }

  if((pm_get_fw_ver(&aux) == PM_ERROR) || (aux != PM_EXPECTED_VERSION)) {
    printf("PM: unexpected version 0x%02X\n", aux);
  }
  printf("PM: firmware version 0x%02X OK\n", aux);

  while(1) {

    etimer_set(&et, CLOCK_SECOND);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    /*-------------------voltage VBAT --------------*/
    if(pm_get_voltage(&voltage) != PM_SUCCESS) {
      printf("PM: error retrieving voltage\n");
    } else {
      printf("%u.%u V\n", voltage / 100, voltage % 100);
    }
  }

#else
  #pragma message "This example is only for Revision B platform"
#endif /* CONTIKI_BOARD_REMOTE_REVB */

  PROCESS_END();
}

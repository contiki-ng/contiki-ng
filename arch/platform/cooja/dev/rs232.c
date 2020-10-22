/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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

#include "lib/sensors.h"
#include "dev/rs232.h"
#include "dev/serial-line.h"
#include "lib/simEnvChange.h"
#include <string.h>
#include <stdio.h>

const struct simInterface rs232_interface;

#ifndef SERIAL_BUF_SIZE
#define SERIAL_BUF_SIZE 2048
#endif

// COOJA variables
char simSerialReceivingData[SERIAL_BUF_SIZE];

/* this mark surely > any buffersize, so it stop receiving even by old cooja
 * new cooja use this marked value to evaluate recv buffer size - in LSB bits
 *
 * cooja RS232 interface can write receving data even when mote not started.
 * So mote starts, and it alredy have data in recv buffer, even before uart device init.
 *
 * current contiki device buffer size (default 2048) is much less, vs cooja used (16k).
 * Since recv buffer restricted by cooja there may happen buffer overload, with
 * consequenced corruption of memory. To prevent it, new cooja RS232 nterface
 * evaluates recv buffer size from value provided by mote.
 * */
#define SERIAL_BUF_STOP 0x10000
int simSerialReceivingLength = SERIAL_BUF_SIZE | SERIAL_BUF_STOP;
char simSerialReceivingFlag  = 0;

char simSerialSendData[SERIAL_BUF_SIZE];
int simSerialSendLength;
char simSerialSendFlag;

static int (* input_handler)(unsigned char) = NULL;

/*-----------------------------------------------------------------------------------*/
void rs232_init(void) {
    // enable receive, think that cooja alredy get SERIAL_BUF_SIZE
    simSerialReceivingLength = 0;
}
/*-----------------------------------------------------------------------------------*/
void rs232_set_speed(unsigned char speed) { }
/*-----------------------------------------------------------------------------------*/
void
rs232_set_input(int (*f)(unsigned char))
{
  input_handler = f;
}
/*-----------------------------------------------------------------------------------*/
void rs232_send(char c) {
    if ( (simSerialSendLength) + 1 > SERIAL_BUF_SIZE) {
      /* Dropping message due to buffer overflow */
      return;
    }

    simSerialSendData[simSerialSendLength] = c;
    simSerialSendLength += 1;
    simSerialSendFlag = 1;
}
/*-----------------------------------------------------------------------------------*/
void
rs232_print(char *message)
{
    unsigned len = strlen(message);
    if ( (simSerialSendLength + len) > SERIAL_BUF_SIZE) {
      /* Dropping message due to buffer overflow */
      return;
    }

    memcpy(simSerialSendData + simSerialSendLength, message, len);
    simSerialSendLength += len;
    simSerialSendFlag = 1;
}
/*-----------------------------------------------------------------------------------*/
static void
doInterfaceActionsBeforeTick(void)
{
  int i;

  if (!simSerialReceivingFlag) {
    return;
  }

  //wait for rs232_init
  if (simSerialReceivingLength > SERIAL_BUF_STOP)
      return;

  if (simSerialReceivingLength == 0) {
    /* Error, should not be zero */
    simSerialReceivingFlag = 0;
    return;
  }

  /* Notify specified rs232 handler */
  if(input_handler != NULL) {
    for (i=0; i < simSerialReceivingLength; i++) {
      input_handler(simSerialReceivingData[i]);
    }
  } else {
    /* Notify serial process */
    for (i=0; i < simSerialReceivingLength; i++) {
      serial_line_input_byte(simSerialReceivingData[i]);
    }
    // intrude \n,  why this is here?
    //serial_line_input_byte(0x0a);
  }

  simSerialReceivingLength = 0;
  simSerialReceivingFlag = 0;
}
/*-----------------------------------------------------------------------------------*/
static void
doInterfaceActionsAfterTick(void)
{
}
/*-----------------------------------------------------------------------------------*/

SIM_INTERFACE(rs232_interface,
	      doInterfaceActionsBeforeTick,
	      doInterfaceActionsAfterTick);

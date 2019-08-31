/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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

/**
 * \file
 *         COOJA Contiki mote main file.
 * \author
 *         Fredrik Osterlind <fros@sics.se>
 */

#include <jni.h>
#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "sys/cc.h"

#include "sys/clock.h"
#include "sys/etimer.h"
#include "sys/cooja_mt.h"

/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Cooja"
#define LOG_LEVEL LOG_LEVEL_MAIN

#include "lib/random.h"
#include "lib/simEnvChange.h"

#include "net/netstack.h"
#include "net/queuebuf.h"

#include "dev/eeprom.h"
#include "dev/serial-line.h"
#include "dev/cooja-radio.h"
#include "dev/button-sensor.h"
#include "dev/pir-sensor.h"
#include "dev/vib-sensor.h"
#include "dev/moteid.h"
#include "dev/button-hal.h"
#include "dev/gpio-hal.h"

#include "sys/node-id.h"
#include "services/rpl-border-router/rpl-border-router.h"
#if BUILD_WITH_ORCHESTRA
#include "orchestra.h"
#endif /* BUILD_WITH_ORCHESTRA */
#if BUILD_WITH_SHELL
#include "serial-shell.h"
#endif /* BUILD_WITH_SHELL */

/* JNI-defined functions, depends on the environment variable CLASSNAME */
#ifndef CLASSNAME
#error CLASSNAME is undefined, required by platform.c
#endif /* CLASSNAME */
#define COOJA__QUOTEME(a,b,c) COOJA_QUOTEME(a,b,c)
#define COOJA_QUOTEME(a,b,c) a##b##c
#define COOJA_JNI_PATH Java_org_contikios_cooja_corecomm_
#define Java_org_contikios_cooja_corecomm_CLASSNAME_init COOJA__QUOTEME(COOJA_JNI_PATH,CLASSNAME,_init)
#define Java_org_contikios_cooja_corecomm_CLASSNAME_getMemory COOJA__QUOTEME(COOJA_JNI_PATH,CLASSNAME,_getMemory)
#define Java_org_contikios_cooja_corecomm_CLASSNAME_setMemory COOJA__QUOTEME(COOJA_JNI_PATH,CLASSNAME,_setMemory)
#define Java_org_contikios_cooja_corecomm_CLASSNAME_tick COOJA__QUOTEME(COOJA_JNI_PATH,CLASSNAME,_tick)
#define Java_org_contikios_cooja_corecomm_CLASSNAME_setReferenceAddress COOJA__QUOTEME(COOJA_JNI_PATH,CLASSNAME,_setReferenceAddress)

#if NETSTACK_CONF_WITH_IPV6
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#endif /* NETSTACK_CONF_WITH_IPV6 */

/* The main function, implemented in contiki-main.c */
int main(void);

/* Simulation mote interfaces */
SIM_INTERFACE_NAME(moteid_interface);
SIM_INTERFACE_NAME(vib_interface);
SIM_INTERFACE_NAME(rs232_interface);
SIM_INTERFACE_NAME(simlog_interface);
SIM_INTERFACE_NAME(beep_interface);
SIM_INTERFACE_NAME(radio_interface);
SIM_INTERFACE_NAME(button_interface);
SIM_INTERFACE_NAME(pir_interface);
SIM_INTERFACE_NAME(clock_interface);
SIM_INTERFACE_NAME(leds_interface);
SIM_INTERFACE_NAME(cfs_interface);
SIM_INTERFACE_NAME(eeprom_interface);
SIM_INTERFACES(&vib_interface, &moteid_interface, &rs232_interface, &simlog_interface, &beep_interface, &radio_interface, &button_interface, &pir_interface, &clock_interface, &leds_interface, &cfs_interface, &eeprom_interface);
/* Example: manually add mote interfaces */
//SIM_INTERFACE_NAME(dummy_interface);
//SIM_INTERFACES(..., &dummy_interface);

/* Sensors */
SENSORS(&button_sensor, &pir_sensor, &vib_sensor);

/*
 * referenceVar is used for comparing absolute and process relative memory.
 * (this must not be static due to memory locations)
 */
long referenceVar;

/*
 * Contiki and rtimer threads.
 */
static struct cooja_mt_thread rtimer_thread;
static struct cooja_mt_thread process_run_thread;
/*---------------------------------------------------------------------------*/
/* Needed since the new LEDs API does not provide this prototype */
void leds_arch_init(void);
/*---------------------------------------------------------------------------*/
static void
rtimer_thread_loop(void *data)
{
  while(1)
  {
    rtimer_arch_check();

    /* Return to COOJA */
    cooja_mt_yield();
  }
}
/*---------------------------------------------------------------------------*/
static void
set_lladdr(void)
{
  linkaddr_t addr;

  memset(&addr, 0, sizeof(linkaddr_t));
#if NETSTACK_CONF_WITH_IPV6
  {
    int i;
    for(i = 0; i < sizeof(uip_lladdr.addr); i += 2) {
      addr.u8[i + 1] = simMoteID & 0xff;
      addr.u8[i + 0] = simMoteID >> 8;
    }
  }
#else /* NETSTACK_CONF_WITH_IPV6 */
  addr.u8[0] = simMoteID & 0xff;
  addr.u8[1] = simMoteID >> 8;
#endif /* NETSTACK_CONF_WITH_IPV6 */
  linkaddr_set_node_addr(&addr);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_one()
{
  gpio_hal_init();
  leds_arch_init();
  return;
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two()
{
  set_lladdr();
  button_hal_init();
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three()
{
  /* Initialize eeprom */
  eeprom_init();
  /* Start serial process */
  serial_line_init();
}
/*---------------------------------------------------------------------------*/
void
platform_main_loop()
{
  while(1)
  {
    simProcessRunValue = process_run();
    while(simProcessRunValue-- > 0) {
      process_run();
    }
    simProcessRunValue = process_nevents();

    /* Check if we must stay awake */
    if(simDontFallAsleep) {
      simDontFallAsleep = 0;
      simProcessRunValue = 1;
    }

    /* Return to COOJA */
    cooja_mt_yield();
  }
}
/*---------------------------------------------------------------------------*/
static void
process_run_thread_loop(void *data)
{
  /* Yield once during bootup */
  simProcessRunValue = 1;
  cooja_mt_yield();

  /* Then call common Contiki-NG main function */
  main();
}
/*---------------------------------------------------------------------------*/
/**
 * \brief      Initialize a mote by starting processes etc.
 * \param env  JNI Environment interface pointer
 * \param obj  unused
 *
 *             This function initializes a mote by starting certain
 *             processes and setting up the environment.
 *
 *             This is a JNI function and should only be called via the
 *             responsible Java part (MoteType.java).
 */
JNIEXPORT void JNICALL
Java_org_contikios_cooja_corecomm_CLASSNAME_init(JNIEnv *env, jobject obj)
{
  /* Create rtimers and Contiki threads */
  cooja_mt_start(&rtimer_thread, &rtimer_thread_loop, NULL);
  cooja_mt_start(&process_run_thread, &process_run_thread_loop, NULL);
 }
/*---------------------------------------------------------------------------*/
/**
 * \brief      Get a segment from the process memory.
 * \param env      JNI Environment interface pointer
 * \param obj      unused
 * \param rel_addr Start address of segment
 * \param length   Size of memory segment
 * \param mem_arr  Byte array destination for the fetched memory segment
 * \return     Java byte array containing a copy of memory segment.
 *
 *             Fetches a memory segment from the process memory starting at
 *             (rel_addr), with size (length). This function does not perform
 *             ANY error checking, and the process may crash if addresses are
 *             not available/readable.
 *
 *             This is a JNI function and should only be called via the
 *             responsible Java part (MoteType.java).
 */
JNIEXPORT void JNICALL
Java_org_contikios_cooja_corecomm_CLASSNAME_getMemory(JNIEnv *env, jobject obj, jint rel_addr, jint length, jbyteArray mem_arr)
{
  (*env)->SetByteArrayRegion(
      env,
      mem_arr,
      0,
      (size_t) length,
      (jbyte *) (((long)rel_addr) + referenceVar)
  );
}
/*---------------------------------------------------------------------------*/
/**
 * \brief      Replace a segment of the process memory with given byte array.
 * \param env      JNI Environment interface pointer
 * \param obj      unused
 * \param rel_addr Start address of segment
 * \param length   Size of memory segment
 * \param mem_arr  Byte array contaning new memory
 *
 *             Replaces a process memory segment with given byte array.
 *             This function does not perform ANY error checking, and the
 *             process may crash if addresses are not available/writable.
 *
 *             This is a JNI function and should only be called via the
 *             responsible Java part (MoteType.java).
 */
JNIEXPORT void JNICALL
Java_org_contikios_cooja_corecomm_CLASSNAME_setMemory(JNIEnv *env, jobject obj, jint rel_addr, jint length, jbyteArray mem_arr)
{
  jbyte *mem = (*env)->GetByteArrayElements(env, mem_arr, 0);
  memcpy((char*) (((long)rel_addr) + referenceVar),
         mem,
         length);
  (*env)->ReleaseByteArrayElements(env, mem_arr, mem, 0);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief      Let mote execute one "block" of code (tick mote).
 * \param env  JNI Environment interface pointer
 * \param obj  unused
 *
 *             Let mote defined by the active contiki processes and current
 *             process memory execute some program code. This code must not block
 *             or else this function will never return. A typical contiki
 *             process will return when it executes PROCESS_WAIT..() statements.
 *
 *             Before the control is left to contiki processes, any messages
 *             from the Java part are handled. These may for example be
 *             incoming network data. After the contiki processes return control,
 *             messages to the Java part are also handled (those which may need
 *             special attention).
 *
 *             This is a JNI function and should only be called via the
 *             responsible Java part (MoteType.java).
 */
JNIEXPORT void JNICALL
Java_org_contikios_cooja_corecomm_CLASSNAME_tick(JNIEnv *env, jobject obj)
{
  simProcessRunValue = 0;

  /* Let all simulation interfaces act first */
  doActionsBeforeTick();

  /* Poll etimer process */
  if(etimer_pending()) {
    etimer_request_poll();
  }

  /* Let rtimers run.
   * Sets simProcessRunValue */
  cooja_mt_exec(&rtimer_thread);

  if(simProcessRunValue == 0) {
    /* Rtimers done: Let Contiki handle a few events.
     * Sets simProcessRunValue */
    cooja_mt_exec(&process_run_thread);
  }

  /* Let all simulation interfaces act before returning to java */
  doActionsAfterTick();

  /* Do we have any pending timers */
  simEtimerPending = etimer_pending();

  /* Save nearest expiration time */
  simEtimerNextExpirationTime = etimer_next_expiration_time();

}
/*---------------------------------------------------------------------------*/
/**
 * \brief      Set the relative memory address of the reference variable.
 * \param env  JNI Environment interface pointer
 * \param obj  unused
 * \param addr Relative memory address
 *
 *             This is a JNI function and should only be called via the
 *             responsible Java part (MoteType.java).
 */
JNIEXPORT void JNICALL
Java_org_contikios_cooja_corecomm_CLASSNAME_setReferenceAddress(JNIEnv *env, jobject obj, jint addr)
{
  referenceVar = (((long)&referenceVar) - ((long)addr));
}

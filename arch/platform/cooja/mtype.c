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
 *         COOJA Contiki mote type file.
 * \author
 *         Fredrik Osterlind <fros@sics.se>
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "sys/cc.h"
#include "sys/cooja_mt.h"
#include <jni.h>

/* JNI-defined functions, depends on the environment variable CLASSNAME */
#ifndef CLASSNAME
#error CLASSNAME is undefined, required by mtype.c
#endif /* CLASSNAME */
/* Construct the name of JNI method m in class c. */
#define COOJA_METHOD(c, m) COOJA_QUOTEME(c, m)
/* Indirection to get the right preprocessor behavior. */
#define COOJA_QUOTEME(c, m) Java_org_contikios_cooja_corecomm_##c##_##m
/* Names of JNI methods. */
#define CLASS_init COOJA_METHOD(CLASSNAME, init)
#define CLASS_getMemory COOJA_METHOD(CLASSNAME, getMemory)
#define CLASS_setMemory COOJA_METHOD(CLASSNAME, setMemory)
#define CLASS_tick COOJA_METHOD(CLASSNAME, tick)
#define CLASS_setReferenceAddress COOJA_METHOD(CLASSNAME, setReferenceAddress)

/* The main function, implemented in contiki-main.c */
int main(void);

/*
 * referenceVar is used for comparing absolute and process relative memory.
 * (this must not be static due to memory locations)
 */
intptr_t referenceVar;

/*
 * Contiki and rtimer threads.
 */
static struct cooja_mt_thread rtimer_thread;
static struct cooja_mt_thread process_run_thread;
/*---------------------------------------------------------------------------*/
static void
rtimer_thread_loop(void *data)
{
  while(1) {
    rtimer_arch_check();

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
 * \brief           Callback on load of library.
 * \param vm        unused
 * \param reserved  unused
 *
 * This function is required to return at least the JNI version for
 * the functions we use.
 *
 * Java 11 is the oldest supported Java version so the function returns
 * JNI_VERSION_10 for now.
 */
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
  return JNI_VERSION_10;
}
/*---------------------------------------------------------------------------*/
void
cooja_init(void)
{
  /* Create rtimers and Contiki threads */
  cooja_mt_start(&rtimer_thread, &rtimer_thread_loop, NULL);
  cooja_mt_start(&process_run_thread, &process_run_thread_loop, NULL);
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
CLASS_init(JNIEnv *env, jobject obj)
{
  cooja_init();
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
CLASS_getMemory(JNIEnv *env, jobject obj, jlong rel_addr, jint length,
                jbyteArray mem_arr)
{
  (*env)->SetByteArrayRegion(
      env,
      mem_arr,
      0,
      (size_t) length,
      (jbyte *) (((intptr_t)rel_addr) + referenceVar)
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
CLASS_setMemory(JNIEnv *env, jobject obj, jlong rel_addr, jint length,
                jbyteArray mem_arr)
{
  (*env)->GetByteArrayRegion(env, mem_arr, 0, length,
                             (jbyte *)((intptr_t)rel_addr + referenceVar));
}
/*---------------------------------------------------------------------------*/
void
cooja_tick(void)
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
CLASS_tick(JNIEnv *env, jobject obj)
{
  cooja_tick();
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
CLASS_setReferenceAddress(JNIEnv *env, jobject obj, jlong addr)
{
  referenceVar = (((intptr_t)&referenceVar) - ((intptr_t)addr));
}
/*---------------------------------------------------------------------------*/

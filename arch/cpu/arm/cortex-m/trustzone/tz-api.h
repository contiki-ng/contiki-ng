/*
 * Copyright (c) 2023, RISE Research Institutes of Sweden
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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

/*
 * \file
 *   TrustZone API for communication between zones.
 * \author
 *   Niclas Finne <niclas.finne@ri.se>
 *   Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

/** \addtogroup arm
 * @{
 */

/**
 * \defgroup trustzone TrustZone for Arm Cortex-M
 *
 * This subsystem implements TrustZone support for Arm Cortex-M
 * processors. The archtiecture is based on dual Contiki-NG firmwares:
 * the secure world contains an instance of Contiki-NG with reduced
 * functionality, and the normal world contains an instance with
 * regular functionality. When programming an IoT device, the hex
 * files with the two firmwares are merged into a single hex file,
 * which is flashed to the device.
 *
 * Both worlds can access core system functionality such as processes,
 * timers, and library functions. The normal world is expected to
 * contain applications and networking functionality. By contrast, the
 * secure world will contain secret information and functionality for
 * monitoring the normal world. Hardware peripherals can be configured
 * to be accessible in either of the worlds.
 *
 * Currently, the only supported Contiki-NG platform is the nRF5340
 * development kit, which is equipped with two different Arm
 * Cortex-M33 processors.
 *
 * @{
 */

#ifndef TZ_API_H
#define TZ_API_H

#include <stdbool.h>
#include <stdlib.h>

#ifdef TRUSTZONE_SECURE

/**
 * The CC_TRUSTZONE_SECURE_CALL marks a function in the secure world
 * as being possible to call from the normal world. When executing
 * such a function, the processor will be in secure state.
 */
#define CC_TRUSTZONE_SECURE_CALL __attribute__((cmse_nonsecure_entry))

/**
 * The CC_TRUSTZONE_NONSECURE_CALL marks a function in the normal
 * world as being possible to call from the secure world. When
 * executing such a function, the processor will be in non-secure
 * state.
 */
#define CC_TRUSTZONE_NONSECURE_CALL __attribute__((cmse_nonsecure_call))

/**
 * The trustzone_init_event is posted to all automatically started
 * processes when both the secure world and the normal world have
 * finished their initialization.
 *
 * This allows user processes to wait until they can begin their
 * execution of tasks that may depend on TrustZone-specific
 * functionality.
 */
extern process_event_t trustzone_init_event;

/**
 * Linker symbols.
 */

/* End of the text region. */
extern uint32_t __etext;

/* Start of the Secure Gateway region. */
extern uint32_t __sg_start;

/* End of the Secure Gateway region. */
extern uint32_t __sg_end;

/* Secure Gateway region size, aligned to the next 32 byte boundary. */
extern uint32_t __nsc_size;
/******************************************************************************/

#else /* TRUSTZONE_SECURE */

#define CC_TRUSTZONE_NONSECURE_CALL

#endif /* TRUSTZONE_SECURE */

typedef bool (*ns_poll_t)(void) CC_TRUSTZONE_NONSECURE_CALL;

struct tz_api {
  ns_poll_t request_poll;
};

/**
 * \brief        Initialize the TrustZone API.
 * \param apip   A pointer to a tz_api structure.
 * \retval false Error (apip pointed to invalid memory,
 *               or the API has been initialized already.)
 * \retval true  Success.
 */
bool tz_api_init(struct tz_api *apip);

/**
 * \brief        Poll the secure world and process all events in the queue.
 * \retval true  If the secure world has more events to process.
 * \retval false If the secure world has no more events to process.
 *
 */
bool tz_api_poll(void);

/**
 * \brief        Print the specified message via the secure world.
 *
 */
void tz_api_println(const char *text);

/**
 * \brief        Request poll from normal world.
 *
 *               Only called from secure world.
 */
bool tz_api_request_poll_from_ns(void);

#endif /* !TZ_API_H */
/** @} */
/** @} */

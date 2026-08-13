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

/**
 * \brief Pend an NS-targeted IRQ to wake the normal world.
 *
 *        Called from secure context (including secure ISRs) by
 *        tz_api_request_ns_poll after setting ns_poll_pending.
 *        Implemented by the platform; weakly defined as a no-op in
 *        tz-api.c so platforms without a wake mechanism still link.
 */
void tz_arch_signal_ns(void);
/******************************************************************************/

#else /* TRUSTZONE_SECURE */

#define CC_TRUSTZONE_SECURE_CALL
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
 *
 * \note         Must be called from the normal world before any
 *               normal-world scheduling begins, since the secure side
 *               posts trustzone_init_event to autostart processes
 *               from inside this call.
 */
bool tz_api_init(struct tz_api *apip);

/**
 * \brief        Poll the secure world and process all events in the queue.
 * \retval true  If the secure world has more work to do — either residual
 *               events in the queue, or a deferred poll request raised by
 *               the secure side during the call. The NS caller should
 *               reschedule itself.
 * \retval false If the secure world has nothing more to do, or the call
 *               was rejected (see note).
 *
 * \note         Must be called only from NS thread mode. The function
 *               runs process_run() and is not reentrant; calls from a
 *               handler context (NS interrupt or, defensively, a
 *               secure ISR) are rejected and return false without
 *               running events.
 */
bool tz_api_poll(void);

/**
 * \brief        Print the specified message via the secure world.
 * \param text   A pointer to the message text in non-secure memory.
 * \param len    The length of the message in bytes.
 */
void tz_api_println(const char *text, size_t len);

/**
 * \brief        Mark the normal world as needing another poll cycle.
 *
 *               Called from the secure world (e.g. via the Contiki-NG
 *               process module's PROCESS_CONF_POLL_REQUESTED hook)
 *               when secure-side state changes that the normal world
 *               needs to react to. The flag is observed by the next
 *               tz_api_poll(), which then returns true so the NS
 *               caller reschedules itself.
 *
 *               This is a secure-internal helper, not a secure
 *               gateway entry, and must not be called from the
 *               normal world.
 */
bool tz_api_request_ns_poll(void);

#endif /* !TZ_API_H */
/** @} */
/** @} */

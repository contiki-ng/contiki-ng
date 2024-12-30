/*
 * Copyright (c) 2022 RISE Research Institutes of Sweden
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Authors: John Kanwar <johnkanwar@hotmail.com>
 *          Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 *          Niclas Finne <niclas.finne@ri.se>
 */

#include "contiki.h"
#include "region_defs.h"
#include "spu.h"
#include "tz-api.h"
#include "tz-fault.h"
#include "tz-target-cfg.h"

#include <arm_cmse.h>

/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "TZSecureWorld"
#define LOG_LEVEL LOG_LEVEL_INFO
/*---------------------------------------------------------------------------*/
typedef void __attribute__((cmse_nonsecure_call)) (*tz_ns_func_ptr_t)(void);
#define TZ_NONSECURE_FUNC_PTR_DECLARE(fptr) tz_ns_func_ptr_t fptr
#define TZ_NONSECURE_FUNC_PTR_CREATE(fptr) \
  ((tz_ns_func_ptr_t)(cmse_nsfptr_create(fptr)))
/*---------------------------------------------------------------------------*/
static uintptr_t *
setup(void)
{
  LOG_INFO("Initializing TrustZone\n");

  /* SPM example */
  LOG_INFO("Enabling fault handlers: ");
  if(enable_fault_handlers() == TFM_PLAT_ERR_SUCCESS) {
    LOG_INFO_("success\n");
  } else {
    LOG_INFO_("failure\n");
    return NULL;
  }

  /*
   * Set flash and RAM secure.
   * Set non-secure partition non-secure for both flash and RAM.
   * Set all peripherals non-secure.
   */
  sau_and_idau_cfg();
  non_secure_configuration();

  /* Verify that the start of the vector table of the non-secure world
     now has non-secure permissions. */
  void *ptr = (void *)NS_CODE_START;
  if(cmse_check_address_range(ptr, sizeof(ptr), CMSE_NONSECURE) == ptr) {
    /* Check succeeded. */
    LOG_INFO("Non-secure image has correct permissions\n");
  } else {
    LOG_ERR("Non-secure image has incorrect permissions\n");
    return NULL;
  }

  enum tfm_plat_err_t tfm_err = nvic_interrupt_target_state_cfg();
  if(tfm_err != TFM_PLAT_ERR_SUCCESS) {
    LOG_DBG("Interrupt state: 0x%x\n", tfm_err);
  }

  uintptr_t *vtor_ns = (uintptr_t *)NS_CODE_START;
  LOG_DBG("NS image at %p\n", vtor_ns);
  LOG_DBG("NS main stack pointer at 0x%"PRIxPTR"\n", vtor_ns[0]);
  LOG_DBG("NS reset vector at 0x%"PRIxPTR"\n", vtor_ns[1]);

  /*
   * Initialize the Non-Secure Callable (NSC) region in order to enable
   * function calls from the non-secure world to the secure world.
   */
  LOG_DBG("Secure gateway region: %p - %p\n", &__sg_start, &__sg_end);
  LOG_DBG("NSC size %p\n", &__nsc_size);
  spu_regions_flash_config_non_secure_callable((uint32_t)&__sg_start,
					       (uint32_t)&__sg_end - 1);

  LOG_DBG("__ARM_FEATURE_CMSE = %d\n", __ARM_FEATURE_CMSE);

  /* Configure non-secure stack */
  tz_nonsecure_setup_conf_t spm_ns_conf = {
      .vtor_ns = (uintptr_t)vtor_ns,
      .msp_ns = vtor_ns[0],
      .psp_ns = vtor_ns[0],  /* Was: 0 */
      .control_ns.npriv = 0, /* Privileged mode */
      .control_ns.spsel = 0, /* Use MSP in Thread mode */
  };

  tz_nonsecure_state_setup(&spm_ns_conf);

  return vtor_ns;
}
/*---------------------------------------------------------------------------*/
void
platform_main_loop(void)
{
  tz_fault_init();

  /* Process all events before switching to non-secure */
  process_num_events_t r;
  do {
    r = process_run();
    watchdog_periodic();
  } while(r > 0);

  uintptr_t *vtor_ns = setup();
  if(vtor_ns == NULL) {
    LOG_ERR("Not jumping to the normal world due to initialization error\n");
    return;
  }

  TZ_NONSECURE_FUNC_PTR_DECLARE(reset_ns);
  reset_ns = TZ_NONSECURE_FUNC_PTR_CREATE(vtor_ns[1]);
  if(!cmse_is_nsfptr(reset_ns)) {
    LOG_ERR("Invalid non-secure pointer type\n");
    return;
  }

  LOG_INFO("Preparing to jump to the normal world\n");
  spu_periph_config_uarte();

  __DSB();
  __ISB();
  reset_ns();
}
/*---------------------------------------------------------------------------*/

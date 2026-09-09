/*
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup nrf-platforms
 * @{
 *
 * \file
 *      Platform implementation for nRF
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "dev/gpio-hal.h"
#include "dev/button-hal.h"
#include "dev/leds.h"
#include "dev/serial-line.h"
#include "dev/xmem.h"
#include "lib/csprng.h"

#include "int-master.h"
#include "sensors.h"
#include "uarte-arch.h"
#include "linkaddr-arch.h"
#include "reset-arch.h"

#include "lpm.h"
#include "nrfx_config.h"
#include "usb.h"

/* Pulls in the SoC feature macros, such as NVMC_FEATURE_CACHE_PRESENT. */
#include "nrf_peripherals.h"

/*
 * The cache mechanism available to this build, if any. Both the dedicated
 * cache peripheral and the NVMC-embedded cache are secure-only on
 * TrustZone parts, so neither is defined for a normal-world image;
 * consequently NRF_CONF_ICACHE_ENABLE takes effect in the secure-world
 * build only. Everything downstream (the #include below and the
 * icache_init() branch) tests only these two derived macros, never the
 * raw peripheral-presence macros, so the two selections cannot drift.
 */
#if !defined(NRF_TRUSTZONE_NONSECURE)
#if defined(NRF_ICACHE)
#define NRF_CODE_CACHE NRF_ICACHE /* nRF54L series */
#elif defined(NRF_CACHE)
#define NRF_CODE_CACHE NRF_CACHE /* nRF5340 application core */
#elif defined(NVMC_FEATURE_CACHE_PRESENT)
#define NRF_NVMC_CODE_CACHE NRF_NVMC /* nRF52840, nRF5340 network core */
#endif
#endif /* !defined(NRF_TRUSTZONE_NONSECURE) */

#if defined(NRF_CODE_CACHE)
#include "hal/nrf_cache.h"
#elif defined(NRF_NVMC_CODE_CACHE)
#include "hal/nrf_nvmc.h"
#endif

/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "NRF"
#define LOG_LEVEL LOG_LEVEL_MAIN
/*---------------------------------------------------------------------------*/
/*
 * Only the SoCs with an ICACHE or similar peripheral define this in their
 * CPU configuration header, so provide an off default for the others.
 */
#ifndef NRF_CONF_ICACHE_ENABLE
#define NRF_CONF_ICACHE_ENABLE 0
#endif

/*---------------------------------------------------------------------------*/
#if NRF_HARDFAULT_HANDLER_EXTENDED
void hardfault_print_saved_crash(void);
#endif
/*---------------------------------------------------------------------------*/
__attribute__((weak)) void
platform_init_board(void)
{
}
/*---------------------------------------------------------------------------*/
__attribute__((weak)) void
platform_init_board_stage_two(void)
{
}
/*---------------------------------------------------------------------------*/
static void
icache_init(void)
{
#if NRF_CONF_ICACHE_ENABLE
#if defined(NRF_CODE_CACHE)
  /*
   * Enable the code cache, on SoCs that have the peripheral and have
   * not opted out (see NRF_CONF_ICACHE_ENABLE). Enabling performs no
   * invalidation of its own, and the reset state of the cache memory is
   * not documented, so invalidate first rather than risk serving a line
   * left over from code that a firmware update has since rewritten. The
   * invalidate task runs asynchronously; wait for it before enabling.
   */
  nrf_cache_invalidate(NRF_CODE_CACHE);
#if NRF_CACHE_HAS_STATUS
  while(nrf_cache_busy_check(NRF_CODE_CACHE)) {
  }
#endif /* NRF_CACHE_HAS_STATUS */
  nrf_cache_enable(NRF_CODE_CACHE);
#elif defined(NRF_NVMC_CODE_CACHE)
  /*
   * The nRF52840 and the nRF5340 network core cache instruction fetches in
   * the NVMC rather than in a separate cache peripheral. ICACHECNF resets
   * to disabled, but that only covers power-on reset: a watchdog reset or
   * the dongle's DFU reset path also leaves ICACHECNF at 0 without the
   * cache RAM's contents being documented as cleared. Disabling is the
   * documented invalidate for this cache, so do it before enabling rather
   * than trust the reset state. Go through the HAL rather than writing
   * ICACHECNF directly; it carries the workaround for nRF53 anomaly 6 on
   * the disable path.
   */
  nrf_nvmc_icache_config_set(NRF_NVMC_CODE_CACHE, NRF_NVMC_ICACHE_DISABLE);
  nrf_nvmc_icache_config_set(NRF_NVMC_CODE_CACHE, NRF_NVMC_ICACHE_ENABLE);
#endif
#endif /* NRF_CONF_ICACHE_ENABLE */
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_one(void)
{
  icache_init();
  gpio_hal_init();
  platform_init_board();
  leds_init();
}
/*---------------------------------------------------------------------------*/
static void
feed_csprng(void)
{
#if defined(NRF_RNG) && CSPRNG_ENABLED
  struct csprng_seed seed;

  NRF_RNG->TASKS_START = 1;
  for(size_t i = 0; i < sizeof(seed); i++) {
    NRF_RNG->EVENTS_VALRDY = 0;
    while(!NRF_RNG->EVENTS_VALRDY);
    ((uint8_t *)&seed)[i] = NRF_RNG->VALUE;
  }
  NRF_RNG->TASKS_STOP = 1;
  csprng_feed(&seed);
#endif /* defined(NRF_RNG) && CSPRNG_ENABLED */
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two(void)
{
  platform_init_board_stage_two();

  /*
   * There are two images of everything when building with TrustZone, and the
   * normal world is the one that owns the buttons, through the non-secure
   * GPIOTE instance. Initializing button-hal in the secure image as well would
   * arm the secure instance on the same pins, and a button press would then
   * raise a secure interrupt that nothing services, because the secure image
   * has handed over to the normal world and no longer runs a scheduler.
   */
#if !defined(NRF_TRUSTZONE_SECURE)
  button_hal_init();
#endif

  /* There are two images of everything when building with
   * TrustZone, and uarte can only be initialized once,
   * so initialize in the secure mode. */
#if NRF_HAS_UARTE && !defined(NRF_TRUSTZONE_NONSECURE)
  uarte_init();
#if NRF_HARDFAULT_HANDLER_EXTENDED
  hardfault_print_saved_crash();
#endif
#endif /* NRF_HAS_UARTE */

  /*
   * The RRAMC behind xmem is a secure peripheral, so with TrustZone only
   * the secure image initializes it, as with the UARTE above.
   */
#if BUILD_WITH_XMEM && !defined(NRF_TRUSTZONE_NONSECURE)
  xmem_init();
#endif

#if NRF_HAS_USB && defined(NRF_NATIVE_USB) && NRF_NATIVE_USB == 1
  usb_init();
#endif /* NRF_HAS_USB && defined(NRF_NATIVE_USB) && NRF_NATIVE_USB == 1 */

  serial_line_init();

#if BUILD_WITH_SHELL
#if PLATFORM_DBG_CONF_USB
  usb_set_input(serial_line_input_byte);
#else /* PLATFORM_DBG_CONF_USB */
  uarte_set_input(serial_line_input_byte);
#endif /* PLATFORM_DBG_CONF_USB */
#endif /* BUILD_WITH_SHELL */

  populate_link_address();

  feed_csprng();

  reset_debug();
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three(void)
{
  process_start(&sensors_process, NULL);
}
/*---------------------------------------------------------------------------*/
void
platform_idle()
{
  lpm_drop();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */

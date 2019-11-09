/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
 * Copyright (c) 2016, Mark Solters <msolters@gmail.com>
 * Copyright (c) 2018, George Oikonomou - http://www.spd.gr
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
#include "contiki.h"
#include "dev/leds.h"
#include "dev/gpio-hal.h"
#include "dev/oscillators.h"
#include "dev/soc-rtc.h"
#include "dev/ext-flash/ext-flash.h"
#include "dev/watchdog.h"
#include "net/app-layer/ota/ota.h"
#include "net/app-layer/ota/ota-ext-flash.h"
#include "driverlib/flash.h"
#include "sys/int-master.h"
#include "sys/critical.h"

#include "ti-lib.h"
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "bootloader"
#ifndef LOG_LEVEL_BOOTLOADER
#define LOG_LEVEL_BOOTLOADER LOG_LEVEL_NONE
#endif
#define LOG_LEVEL LOG_LEVEL_BOOTLOADER
/*---------------------------------------------------------------------------*/
#define __STR(x) #x
#define STR(x) __STR(x)
/*---------------------------------------------------------------------------*/
#define FLASH_SECTOR_SIZE 4096
/*---------------------------------------------------------------------------*/
void
bootloader_arch_jump_to_app()
{
  /* Load the address of the vector to R0 */
  __asm(" MOV R0, " STR(OTA_MAIN_FW_BASE));

  /* Load the address of the Reset Handler to R1:
   * Offset by 0x04 from the vector start */
  __asm(" LDR R1, [R0, #0x4]");

  /* Reset the stack pointer */
  __asm(" LDR SP, [R0, #0x0]");

  /* Make sure we are in thumb mode after the jump */
  __asm(" ORR R1, #1");

  /* And jump */
  __asm(" BX R1");
}
/*---------------------------------------------------------------------------*/
void
bootloader_arch_init()
{
  /* Enable flash cache and prefetch. */
  ti_lib_vims_mode_set(VIMS_BASE, VIMS_MODE_ENABLED);
  ti_lib_vims_configure(VIMS_BASE, true, true);

  ti_lib_int_master_disable();

  /* Set the LF XOSC as the LF system clock source */
  ti_lib_osc_clock_source_set(OSC_SRC_CLK_LF, OSC_XOSC_LF);

  /* Wait for LF clock source to become XOSC_LF */
  while(ti_lib_osc_clock_source_get(OSC_SRC_CLK_LF) != OSC_XOSC_LF);

  /* Turn on the PERIPH PD */
  ti_lib_prcm_power_domain_on(PRCM_DOMAIN_PERIPH);

  /* Wait for domains to power on */
  while((ti_lib_prcm_power_domain_status(PRCM_DOMAIN_PERIPH)
         != PRCM_DOMAIN_POWER_ON));

  /* Enable GPIO peripheral */
  ti_lib_prcm_peripheral_run_enable(PRCM_PERIPH_GPIO);

  /* Apply settings and wait for them to take effect */
  ti_lib_prcm_load_set();
  while(!ti_lib_prcm_load_get());

  gpio_hal_init();

  leds_init();

  /*
   * Disable I/O pad sleep mode and open I/O latches in the AON IOC interface
   * This is only relevant when returning from shutdown (which is what froze
   * latches in the first place. Before doing these things though, we should
   * allow software to first regain control of pins
   */
  ti_lib_pwr_ctrl_io_freeze_disable();

  ti_lib_rom_int_enable(INT_AON_GPIO_EDGE);
  ti_lib_int_master_enable();

  soc_rtc_init();
}
/*---------------------------------------------------------------------------*/
static bool
sector_erase(uint32_t write_addr)
{
  int_master_status_t status;
  uint32_t rv;

  watchdog_periodic();

  status = critical_enter();
  rv = FlashSectorErase(write_addr);
  critical_exit(status);
  if(rv != FAPI_STATUS_SUCCESS) {
    LOG_ERR("sector_erase failed at 0x%08lX, value 0x%08lX\n",
            write_addr, rv);
    return false;
  }

  return true;
}
/*---------------------------------------------------------------------------*/
static bool
sector_write(uint8_t *src, uint32_t write_addr)
{
  int_master_status_t status;
  uint32_t rv;

  watchdog_periodic();

  status = critical_enter();
  rv = FlashProgram(src, write_addr, FLASH_SECTOR_SIZE);
  critical_exit(status);
  if(rv != FAPI_STATUS_SUCCESS) {
    LOG_ERR("Failed to write at 0x%08lX, value 0x%08lX\n", write_addr, rv);
    return false;
  }

  return true;
}
/*---------------------------------------------------------------------------*/
void
bootloader_arch_install_image_from_area(uint8_t area)
{
  uint32_t write_addr;
  uint32_t ext_read_addr;
  uint32_t rv;
  uint8_t buf[FLASH_SECTOR_SIZE];

  if(area >= OTA_EXT_FLASH_AREA_COUNT) {
    return;
  }

  if(!ext_flash_open(NULL)) {
    LOG_ERR("Failed to open external flash\n");
    return;
  }

  /*
   * Start at OTA_MAIN_FW_BASE, for all sectors delete and the copy from
   * external flash, except for the last sector which requires special
   * attention in order to retain the CCFG.
   */
  write_addr = OTA_MAIN_FW_BASE;
  ext_read_addr = area * OTA_EXT_FLASH_AREA_LEN;
  while(write_addr < INTERNAL_FLASH_LENGTH - FLASH_SECTOR_SIZE) {
    /* Erase sector */
    if(!sector_erase(write_addr)) {
      return;
    }

    /* Read external flash */
    memset(buf, 0, FLASH_SECTOR_SIZE);
    rv = ext_flash_read(NULL, ext_read_addr, FLASH_SECTOR_SIZE, buf);
    if(!rv) {
      LOG_ERR("Failed to read ext flash address 0x%08lX\n", ext_read_addr);
    }

    /* Write new sector */
    if(!sector_write(buf, write_addr)) {
      return;
    }

    write_addr += FLASH_SECTOR_SIZE;
    ext_read_addr += FLASH_SECTOR_SIZE;
  }

  /*
   * Last sector requires special treatment
   *
   * We will first read the remaining data on external flash and store it in
   * our buffer. We will then add to the buffer the current value of the CCFG
   * area. We will erase the sector and write data + CCFG in one go.
   */

  /* Read CCFG. Destination in buffer: FLASH_SECTOR_SIZE - CCFG_LENGTH */
  memset(buf, 0, FLASH_SECTOR_SIZE);
  LOG_INFO("Read CCFG 0x%02x bytes at 0x%08lX\n", CCFG_LENGTH,
           (unsigned long)CCFG_ABS_ADDR);
  memcpy(&buf[FLASH_SECTOR_SIZE - CCFG_LENGTH],
         (const void *)CCFG_ABS_ADDR, CCFG_LENGTH);

  /* Read the rest FLASH_SECTOR_SIZE - CCFG_LENGTH data from ext flash. */
  rv = ext_flash_read(NULL, ext_read_addr, FLASH_SECTOR_SIZE - CCFG_LENGTH,
                      buf);
  if(!rv) {
    LOG_ERR("Failed to read ext flash address 0x%08lX\n", ext_read_addr);
  }

  /* Erase sector */
  if(!sector_erase(write_addr)) {
    return;
  }

  /* Write the entire thing back */
  if(!sector_write(buf, write_addr)) {
    return;
  }

  ext_flash_close(NULL);
}
/*---------------------------------------------------------------------------*/

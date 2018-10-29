/*
 * Copyright (c) 2017, University of Bristol - http://www.bristol.ac.uk/
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "contiki.h"
#include "ti-lib.h"
#include "dev/spi.h"
#include "sys/mutex.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct spi_locks_s {
  mutex_t lock;
  const spi_device_t *owner;
} spi_locks_t;

/* One lock per SPI controller */
spi_locks_t board_spi_locks_spi[SPI_CONTROLLER_COUNT] = { { MUTEX_STATUS_UNLOCKED, NULL } };

/*---------------------------------------------------------------------------*/
/* Arch-specific properties of each SPI controller */
typedef struct board_spi_controller_s {
  uint32_t ssi_base;
  uint32_t power_domain;
  uint32_t prcm_periph;
  uint32_t ssi_clkgr_clk_en;
} board_spi_controller_t;

static const board_spi_controller_t spi_controller[SPI_CONTROLLER_COUNT] = {
  {
    .ssi_base = SSI0_BASE,
    .power_domain = PRCM_DOMAIN_SERIAL,
    .prcm_periph = PRCM_PERIPH_SSI0,
    .ssi_clkgr_clk_en = PRCM_SSICLKGR_CLK_EN_SSI0
  },
  {
    .ssi_base = SSI1_BASE,
    .power_domain = PRCM_DOMAIN_PERIPH,
    .prcm_periph = PRCM_PERIPH_SSI1,
    .ssi_clkgr_clk_en = PRCM_SSICLKGR_CLK_EN_SSI1
  }
};
/*---------------------------------------------------------------------------*/
bool
spi_arch_has_lock(const spi_device_t *dev)
{
  if(board_spi_locks_spi[dev->spi_controller].owner == dev) {
    return true;
  }

  return false;
}
/*---------------------------------------------------------------------------*/
bool
spi_arch_is_bus_locked(const spi_device_t *dev)
{
  if(board_spi_locks_spi[dev->spi_controller].lock == MUTEX_STATUS_LOCKED) {
    return true;
  }

  return false;
}
/*---------------------------------------------------------------------------*/
static uint32_t
get_mode(const spi_device_t *dev)
{
  /* Select the correct SPI mode */
  if(dev->spi_pha == 0 && dev->spi_pol == 0) {
    return SSI_FRF_MOTO_MODE_0;
  } else if(dev->spi_pha != 0 && dev->spi_pol == 0) {
    return SSI_FRF_MOTO_MODE_1;
  } else if(dev->spi_pha == 0 && dev->spi_pol != 0) {
    return SSI_FRF_MOTO_MODE_2;
  } else {
    return SSI_FRF_MOTO_MODE_3;
  }
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_arch_lock_and_open(const spi_device_t *dev)
{
  uint32_t c;

  /* Lock the SPI bus */
  if(mutex_try_lock(&board_spi_locks_spi[dev->spi_controller].lock) == false) {
    return SPI_DEV_STATUS_BUS_LOCKED;
  }

  board_spi_locks_spi[dev->spi_controller].owner = dev;

  /* CS pin configuration */
  ti_lib_ioc_pin_type_gpio_output(dev->pin_spi_cs);

  /* First, make sure the SERIAL PD is on */
  ti_lib_prcm_power_domain_on(spi_controller[dev->spi_controller].power_domain);
  while((ti_lib_prcm_power_domain_status(spi_controller[dev->spi_controller].power_domain)
         != PRCM_DOMAIN_POWER_ON)) ;

  /* Enable clock in active mode */
  ti_lib_prcm_peripheral_run_enable(spi_controller[dev->spi_controller].prcm_periph);
  ti_lib_prcm_load_set();
  while(!ti_lib_prcm_load_get()) ;

  /* SPI configuration */
  ti_lib_ssi_int_disable(spi_controller[dev->spi_controller].ssi_base, SSI_RXOR | SSI_RXFF | SSI_RXTO | SSI_TXFF);
  ti_lib_ssi_int_clear(spi_controller[dev->spi_controller].ssi_base, SSI_RXOR | SSI_RXTO);
  
  ti_lib_ssi_config_set_exp_clk(spi_controller[dev->spi_controller].ssi_base,
                                ti_lib_sys_ctrl_clock_get(),
                                get_mode(dev), SSI_MODE_MASTER,
                                dev->spi_bit_rate, 8);
  ti_lib_ioc_pin_type_ssi_master(spi_controller[dev->spi_controller].ssi_base,
                                 dev->pin_spi_miso,
                                 dev->pin_spi_mosi, IOID_UNUSED,
                                 dev->pin_spi_sck);

  ti_lib_ssi_enable(spi_controller[dev->spi_controller].ssi_base);

  /* Get rid of residual data from SSI port */
  while(ti_lib_ssi_data_get_non_blocking(spi_controller[dev->spi_controller].ssi_base, &c)) ;

  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_arch_close_and_unlock(const spi_device_t *dev)
{
  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_NOT_OWNED;
  }

  /* Power down SSI */
  ti_lib_prcm_peripheral_run_disable(spi_controller[dev->spi_controller].prcm_periph);
  ti_lib_prcm_load_set();
  while(!ti_lib_prcm_load_get()) ;

  /* Restore pins to a low-consumption state */
  ti_lib_ioc_pin_type_gpio_input(dev->pin_spi_miso);
  ti_lib_ioc_io_port_pull_set(dev->pin_spi_miso, IOC_IOPULL_DOWN);

  ti_lib_ioc_pin_type_gpio_input(dev->pin_spi_mosi);
  ti_lib_ioc_io_port_pull_set(dev->pin_spi_mosi, IOC_IOPULL_DOWN);

  ti_lib_ioc_pin_type_gpio_input(dev->pin_spi_sck);
  ti_lib_ioc_io_port_pull_set(dev->pin_spi_sck, IOC_IOPULL_DOWN);

  /* Unlock the SPI bus */
  board_spi_locks_spi[dev->spi_controller].owner = NULL;
  mutex_unlock(&board_spi_locks_spi[dev->spi_controller].lock);

  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_arch_transfer(const spi_device_t *dev,
                  const uint8_t *write_buf, int wlen,
                  uint8_t *inbuf, int rlen, int ignore_len)
{
  int i;
  int totlen;
  uint32_t c;

  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_NOT_OWNED;
  }

  if(ti_lib_prcm_power_domain_status(spi_controller[dev->spi_controller].power_domain)
     != PRCM_DOMAIN_POWER_ON) {
    return SPI_DEV_STATUS_CLOSED;
  }

  /* Then check the 'run mode' clock gate */
  if(!(HWREG(PRCM_BASE + PRCM_O_SSICLKGR) & spi_controller[dev->spi_controller].ssi_clkgr_clk_en)) {
    return SPI_DEV_STATUS_CLOSED;
  }

  totlen = MAX(rlen + ignore_len, wlen);

  if(totlen == 0) {
    /* Nothing to do */
    return SPI_DEV_STATUS_OK;
  }

  for(i = 0; i < totlen; i++) {
    c = i < wlen ? write_buf[i] : 0;
    ti_lib_ssi_data_put(spi_controller[dev->spi_controller].ssi_base, (uint8_t)c);
    ti_lib_ssi_data_get(spi_controller[dev->spi_controller].ssi_base, &c);
    if(i < rlen) {
      inbuf[i] = (uint8_t)c;
    }
  }
  while(ti_lib_ssi_data_get_non_blocking(spi_controller[dev->spi_controller].ssi_base, &c)) ;
  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/

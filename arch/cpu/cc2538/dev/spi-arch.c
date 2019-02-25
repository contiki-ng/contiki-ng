/*
 * Copyright (c) 2016-2017, Yanzi Networks.
 * Copyright (c) 2018, University of Bristol.
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

#include <stdint.h>
#include "contiki.h"
#include "reg.h"
#include "dev/spi.h"
#include "gpio-hal-arch.h"
#include "sys/cc.h"
#include "ioc.h"
#include "sys-ctrl.h"
#include "ssi.h"
#include "gpio.h"
#include "sys/log.h"
#include "sys/mutex.h"
/*---------------------------------------------------------------------------*/
/* Log configuration */
#define LOG_MODULE "spi-hal-arch"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
/* Default values for the clock rate divider */
#ifdef SPI_ARCH_CONF_SPI0_CPRS_CPSDVSR
#define SPI_ARCH_SPI0_CPRS_CPSDVSR      SPI_ARCH_CONF_SPI0_CPRS_CPSDVSR
#else
#define SPI_ARCH_SPI0_CPRS_CPSDVSR      2
#endif

#ifdef SPI_ARCH_CONF_SPI1_CPRS_CPSDVSR
#define SPI_ARCH_SPI1_CPRS_CPSDVSR      SPI_ARCH_CONF_SPI1_CPRS_CPSDVSR
#else
#define SPI_ARCH_SPI1_CPRS_CPSDVSR      2
#endif

#if (SPI_ARCH_SPI0_CPRS_CPSDVSR & 1) == 1 || \
     SPI_ARCH_SPI0_CPRS_CPSDVSR < 2 || \
     SPI_ARCH_SPI0_CPRS_CPSDVSR > 254
#error SPI_ARCH_SPI0_CPRS_CPSDVSR must be an even number between 2 and 254
#endif

#if (SPI_ARCH_SPI1_CPRS_CPSDVSR & 1) == 1 || \
     SPI_ARCH_SPI1_CPRS_CPSDVSR < 2 || \
     SPI_ARCH_SPI1_CPRS_CPSDVSR > 254
#error SPI_ARCH_SPI1_CPRS_CPSDVSR must be an even number between 2 and 254
#endif
/*---------------------------------------------------------------------------*/
/* CS set and clear macros */
#define SPIX_CS_CLR(port, pin) GPIO_CLR_PIN(GPIO_PORT_TO_BASE(port), GPIO_PIN_MASK(pin))
#define SPIX_CS_SET(port, pin) GPIO_SET_PIN(GPIO_PORT_TO_BASE(port), GPIO_PIN_MASK(pin))
/*---------------------------------------------------------------------------*/
/*
 * Clock source from which the baud clock is determined for the SSI, according
 * to SSI_CC.CS.
 */
#define SSI_SYS_CLOCK   SYS_CTRL_SYS_CLOCK
/*---------------------------------------------------------------------------*/
typedef struct {
  uint32_t base;
  uint32_t ioc_ssirxd_ssi;
  uint32_t ioc_pxx_sel_ssi_clkout;
  uint32_t ioc_pxx_sel_ssi_txd;
  uint8_t ssi_cprs_cpsdvsr;
} spi_regs_t;
/*---------------------------------------------------------------------------*/
static const spi_regs_t spi_regs[SSI_INSTANCE_COUNT] = {
  {
    .base = SSI0_BASE,
    .ioc_ssirxd_ssi = IOC_SSIRXD_SSI0,
    .ioc_pxx_sel_ssi_clkout = IOC_PXX_SEL_SSI0_CLKOUT,
    .ioc_pxx_sel_ssi_txd = IOC_PXX_SEL_SSI0_TXD,
    .ssi_cprs_cpsdvsr = SPI_ARCH_SPI0_CPRS_CPSDVSR,
  }, {
    .base = SSI1_BASE,
    .ioc_ssirxd_ssi = IOC_SSIRXD_SSI1,
    .ioc_pxx_sel_ssi_clkout = IOC_PXX_SEL_SSI1_CLKOUT,
    .ioc_pxx_sel_ssi_txd = IOC_PXX_SEL_SSI1_TXD,
    .ssi_cprs_cpsdvsr = SPI_ARCH_SPI1_CPRS_CPSDVSR,
  }
};

typedef struct spi_locks_s {
  mutex_t lock;
  const spi_device_t *owner;
} spi_locks_t;

/* One lock per SPI controller */
spi_locks_t board_spi_locks_spi[SPI_CONTROLLER_COUNT] = { { MUTEX_STATUS_UNLOCKED, NULL } };

/*---------------------------------------------------------------------------*/
static void
spix_wait_tx_ready(const spi_device_t *dev)
{
  /* Infinite loop until SR_TNF - Transmit FIFO Not Full */
  while(!(REG(spi_regs[dev->spi_controller].base + SSI_SR) & SSI_SR_TNF));
}
/*---------------------------------------------------------------------------*/
static int
spix_read_buf(const spi_device_t *dev)
{
  return REG(spi_regs[dev->spi_controller].base + SSI_DR);
}
/*---------------------------------------------------------------------------*/
static void
spix_write_buf(const spi_device_t *dev, int data)
{
  REG(spi_regs[dev->spi_controller].base + SSI_DR) = data;
}
/*---------------------------------------------------------------------------*/
static void
spix_wait_eotx(const spi_device_t *dev)
{
  /* wait until not busy */
  while(REG(spi_regs[dev->spi_controller].base + SSI_SR) & SSI_SR_BSY);
}
/*---------------------------------------------------------------------------*/
static void
spix_wait_eorx(const spi_device_t *dev)
{
  /* wait as long as receive is empty */
  while(!(REG(spi_regs[dev->spi_controller].base + SSI_SR) & SSI_SR_RNE));
}
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
spi_status_t
spi_arch_lock_and_open(const spi_device_t *dev)
{
  const spi_regs_t *regs;
  uint32_t scr;
  uint64_t div;

  uint32_t cs_port = PIN_TO_PORT(dev->pin_spi_cs);
  uint32_t cs_pin = PIN_TO_NUM(dev->pin_spi_cs);

  uint32_t clk_port = PIN_TO_PORT(dev->pin_spi_sck);
  uint32_t clk_pin = PIN_TO_NUM(dev->pin_spi_sck);

  uint32_t miso_port = PIN_TO_PORT(dev->pin_spi_miso);
  uint32_t miso_pin = PIN_TO_NUM(dev->pin_spi_miso);

  uint32_t mosi_port = PIN_TO_PORT(dev->pin_spi_mosi);
  uint32_t mosi_pin = PIN_TO_NUM(dev->pin_spi_mosi);

  uint32_t mode = 0;

  /* lock the SPI bus */
  if(mutex_try_lock(&board_spi_locks_spi[dev->spi_controller].lock) == false) {
    return SPI_DEV_STATUS_BUS_LOCKED;
  }

  board_spi_locks_spi[dev->spi_controller].owner = dev;

  /* Set SPI phase */
  if(dev->spi_pha != 0) {
    mode = mode | SSI_CR0_SPH;
  }

  /* Set SPI polarity */
  if(dev->spi_pol != 0) {
    mode = mode | SSI_CR0_SPO;
  }

  /* CS pin configuration */
  GPIO_SOFTWARE_CONTROL(GPIO_PORT_TO_BASE(cs_port),
                        GPIO_PIN_MASK(cs_pin));
  ioc_set_over(cs_port, cs_pin, IOC_OVERRIDE_DIS);
  GPIO_SET_OUTPUT(GPIO_PORT_TO_BASE(cs_port), GPIO_PIN_MASK(cs_pin));
  GPIO_SET_PIN(GPIO_PORT_TO_BASE(cs_port), GPIO_PIN_MASK(cs_pin));

  regs = &spi_regs[dev->spi_controller];

  /* SSI Enable */
  REG(SYS_CTRL_RCGCSSI) |= (1 << dev->spi_controller);

  /* Start by disabling the peripheral before configuring it */
  REG(regs->base + SSI_CR1) = 0;

  /* Set the system clock as the SSI clock */
  REG(regs->base + SSI_CC) = 0;

  /* Set the mux correctly to connect the SSI pins to the correct GPIO pins */
  ioc_set_sel(clk_port, clk_pin, regs->ioc_pxx_sel_ssi_clkout);
  ioc_set_sel(mosi_port, mosi_pin, regs->ioc_pxx_sel_ssi_txd);
  REG(regs->ioc_ssirxd_ssi) = dev->pin_spi_miso;

  /* Put all the SSI gpios into peripheral mode */
  GPIO_PERIPHERAL_CONTROL(GPIO_PORT_TO_BASE(clk_port),
                          GPIO_PIN_MASK(clk_pin));
  GPIO_PERIPHERAL_CONTROL(GPIO_PORT_TO_BASE(mosi_port),
                          GPIO_PIN_MASK(mosi_pin));
  GPIO_PERIPHERAL_CONTROL(GPIO_PORT_TO_BASE(miso_port),
                          GPIO_PIN_MASK(miso_pin));

  /* Disable any pull ups or the like */
  ioc_set_over(clk_port, clk_pin, IOC_OVERRIDE_DIS);
  ioc_set_over(mosi_port, mosi_pin, IOC_OVERRIDE_DIS);
  ioc_set_over(miso_port, miso_pin, IOC_OVERRIDE_DIS);

  /* Configure the clock */
  REG(regs->base + SSI_CPSR) = regs->ssi_cprs_cpsdvsr;

  /* Configure the mode */
  REG(regs->base + SSI_CR0) = mode | (0x07);

  /* Configure the SSI serial clock rate */
  if(!dev->spi_bit_rate) {
    scr = 255;
  } else {
    div = (uint64_t)regs->ssi_cprs_cpsdvsr * dev->spi_bit_rate;
    scr = (SSI_SYS_CLOCK + div - 1) / div;
    scr = MIN(MAX(scr, 1), 256) - 1;
  }
  REG(regs->base + SSI_CR0) = (REG(regs->base + SSI_CR0) & ~SSI_CR0_SCR_M) |
    scr << SSI_CR0_SCR_S;

  /* Enable the SSI */
  REG(regs->base + SSI_CR1) |= SSI_CR1_SSE;

  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_arch_close_and_unlock(const spi_device_t *dev)
{
  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_NOT_OWNED;
  }

  /* Disable SSI */
  REG(SYS_CTRL_RCGCSSI) &= ~(1 << dev->spi_controller);

  /* Unlock the SPI bus */
  board_spi_locks_spi[dev->spi_controller].owner = NULL;
  mutex_unlock(&board_spi_locks_spi[dev->spi_controller].lock);

  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
/* Assumes that checking dev and bus is not NULL before calling this */
spi_status_t
spi_arch_transfer(const spi_device_t *dev,
                  const uint8_t *write_buf, int wlen,
                  uint8_t *inbuf, int rlen, int ignore_len)
{
  int i;
  int totlen;
  uint8_t c;

  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_NOT_OWNED;
  }

  LOG_DBG("SPI: transfer (r:%d,w:%d) ", rlen, wlen);

  if(write_buf == NULL && wlen > 0) {
    return SPI_DEV_STATUS_EINVAL;
  }
  if(inbuf == NULL && rlen > 0) {
    return SPI_DEV_STATUS_EINVAL;
  }

  totlen = MAX(rlen + ignore_len, wlen);

  if(totlen == 0) {
    /* Nothing to do */
    return SPI_DEV_STATUS_OK;
  }

  LOG_DBG_("%c%c%c: %u ", rlen > 0 ? 'R' : '-', wlen > 0 ? 'W' : '-',
          ignore_len > 0 ? 'S' : '-', totlen);

  for(i = 0; i < totlen; i++) {
    spix_wait_tx_ready(dev);
    c = i < wlen ? write_buf[i] : 0;
    spix_write_buf(dev, c);
    LOG_DBG_("%c%02x->", i < rlen ? ' ' : '#', c);
    spix_wait_eotx(dev);
    spix_wait_eorx(dev);
    c = spix_read_buf(dev);
    if(i < rlen) {
      inbuf[i] = c;
    }
    LOG_DBG_("%02x", c);
  }
  LOG_DBG_("\n");

  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/

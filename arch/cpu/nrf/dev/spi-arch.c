/*
 * Copyright (C) 2026 Joakim Eriksson
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
 * \addtogroup nrf-spi
 * @{
 *
 * \file
 *         SPI HAL implementation for the nRF, on top of nrfx_spim.
 *
 *         nrfx is driven in blocking mode (no event handler), which keeps
 *         this layer synchronous as os/dev/spi.h expects. Chip select is
 *         not handed to SPIM: os/dev/spi.c drives it as a GPIO through
 *         spi_select()/spi_deselect(), so ss_pin is left unconnected here
 *         and the pin is configured as a GPIO output instead.
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/spi.h"
#include "dev/gpio-hal.h"
#include "sys/mutex.h"

#if SPI_CONTROLLER_COUNT > 0

#include "nrfx_config.h"
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
#include "nrfx_spim.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <stdint.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "spi-arch"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
/*---------------------------------------------------------------------------*/
/*
 * Map each logical controller index to an nrfx SPIM instance. The ids come
 * from the platform (see spi-arch.h); NRFX_SPIM_INSTANCE() pastes them into
 * NRF_SPIMxx and NRFX_SPIMxx_INST_IDX.
 */
static const nrfx_spim_t spim_instance[SPI_CONTROLLER_COUNT] = {
  NRFX_SPIM_INSTANCE(NRF_SPI_CONF_CONTROLLER0_ID),
#if SPI_CONTROLLER_COUNT > 1
  NRFX_SPIM_INSTANCE(NRF_SPI_CONF_CONTROLLER1_ID),
#endif
#if SPI_CONTROLLER_COUNT > 2
  NRFX_SPIM_INSTANCE(NRF_SPI_CONF_CONTROLLER2_ID),
#endif
};
/*---------------------------------------------------------------------------*/
typedef struct {
  mutex_t lock;
  const spi_device_t *owner;
  bool opened;
} spi_arch_lock_t;

static spi_arch_lock_t bus[SPI_CONTROLLER_COUNT];
/*---------------------------------------------------------------------------*/
/*
 * Staging buffers for transfers that EasyDMA cannot service directly, i.e.
 * a write buffer outside Data RAM, or bytes that must be clocked out and
 * discarded (ignore_len).
 */
static uint8_t stage_tx[NRF_SPI_CHUNK_SIZE];
static uint8_t stage_rx[NRF_SPI_CHUNK_SIZE];
/*---------------------------------------------------------------------------*/
/*
 * Rounds a requested bit rate down to one the controller can actually
 * produce.
 *
 * nrfx rejects an unachievable rate outright (NRFX_ERROR_INVALID_PARAM), so
 * without this a device that asks for its own rated maximum -- 20 MHz for an
 * ENC28J60, say -- fails to open rather than running slightly slower.
 *
 * Rounds down, so a device's rated maximum is not exceeded. The one exception
 * is a request below the slowest rate the controller can produce, which is
 * clamped to that slowest rate because the hardware has nothing lower.
 */
static uint32_t
resolve_bit_rate(const nrfx_spim_t *inst, uint32_t requested)
{
#if NRF_SPIM_HAS_PRESCALER
  uint32_t base = NRFX_SPIM_BASE_FREQUENCY_GET(inst);
  uint32_t min = NRF_SPIM_PRESCALER_MIN_GET(inst->p_reg);
  uint32_t max = NRF_SPIM_PRESCALER_MAX_GET(inst->p_reg);
  uint32_t prescaler;

  if(requested == 0) {
    return base / max;
  }

  /* Round the divisor up so the resulting rate does not exceed the request. */
  prescaler = (base + requested - 1) / requested;

  /* The divisor must be even. */
  prescaler += prescaler & 1;

  if(prescaler < min) {
    prescaler = min + (min & 1);
  }
  if(prescaler > max) {
    prescaler = max - (max & 1);
  }

  return base / prescaler;
#else /* NRF_SPIM_HAS_PRESCALER */
  /* Fixed steps: pick the highest one not above the request. */
  static const uint32_t steps[] = {
    32000000, 16000000, 8000000, 4000000, 2000000, 1000000, 500000, 250000,
    125000
  };
  unsigned i;

  (void)inst;

  for(i = 0; i < sizeof(steps) / sizeof(steps[0]); i++) {
    if(requested >= steps[i]) {
      return steps[i];
    }
  }

  return steps[sizeof(steps) / sizeof(steps[0]) - 1];
#endif /* NRF_SPIM_HAS_PRESCALER */
}
/*---------------------------------------------------------------------------*/
static nrf_spim_mode_t
spi_mode(const spi_device_t *dev)
{
  if(dev->spi_pol == 0) {
    return dev->spi_pha == 0 ? NRF_SPIM_MODE_0 : NRF_SPIM_MODE_1;
  }
  return dev->spi_pha == 0 ? NRF_SPIM_MODE_2 : NRF_SPIM_MODE_3;
}
/*---------------------------------------------------------------------------*/
bool
spi_arch_has_lock(const spi_device_t *dev)
{
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return false;
  }

  return bus[dev->spi_controller].owner == dev;
}
/*---------------------------------------------------------------------------*/
bool
spi_arch_is_bus_locked(const spi_device_t *dev)
{
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return false;
  }

  return bus[dev->spi_controller].owner != NULL;
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_arch_lock_and_open(const spi_device_t *dev)
{
  nrfx_spim_config_t config;
  nrfx_err_t err;
  spi_arch_lock_t *b;

  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return SPI_DEV_STATUS_EINVAL;
  }

  b = &bus[dev->spi_controller];

  if(mutex_try_lock(&b->lock) == false) {
    return SPI_DEV_STATUS_BUS_LOCKED;
  }

  config = (nrfx_spim_config_t)NRFX_SPIM_DEFAULT_CONFIG(
    NRF_GPIO_PIN_MAP(SPI_DEVICE_PORT(sck, dev), dev->pin_spi_sck),
    NRF_GPIO_PIN_MAP(SPI_DEVICE_PORT(mosi, dev), dev->pin_spi_mosi),
    NRF_GPIO_PIN_MAP(SPI_DEVICE_PORT(miso, dev), dev->pin_spi_miso),
    NRF_SPIM_PIN_NOT_CONNECTED);

  config.frequency = resolve_bit_rate(&spim_instance[dev->spi_controller],
                                     dev->spi_bit_rate);
  config.mode = spi_mode(dev);
  /*
   * Pad short writes with zeroes rather than the nrfx default of 0xff, to
   * match the behaviour of the other Contiki-NG SPI arch implementations.
   */
  config.orc = 0x00;

  /* Blocking mode: no event handler. */
  err = nrfx_spim_init(&spim_instance[dev->spi_controller], &config,
                       NULL, NULL);
  if(err != NRFX_SUCCESS) {
    LOG_ERR("nrfx_spim_init failed: 0x%08lx\n", (unsigned long)err);
    mutex_unlock(&b->lock);
    return SPI_DEV_STATUS_CLOSED;
  }

  /*
   * CS is driven by os/dev/spi.c as a plain GPIO. Configure it here and
   * leave it deasserted (high, active low).
   */
  gpio_hal_arch_pin_set_output(SPI_DEVICE_PORT(cs, dev), dev->pin_spi_cs);
  gpio_hal_arch_set_pin(SPI_DEVICE_PORT(cs, dev), dev->pin_spi_cs);

  b->owner = dev;
  b->opened = true;

  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_arch_close_and_unlock(const spi_device_t *dev)
{
  spi_arch_lock_t *b;

  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_NOT_OWNED;
  }

  b = &bus[dev->spi_controller];

  if(b->opened) {
    nrfx_spim_uninit(&spim_instance[dev->spi_controller]);
    b->opened = false;
  }

  b->owner = NULL;
  mutex_unlock(&b->lock);

  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
static spi_status_t
xfer(const nrfx_spim_t *inst, const uint8_t *tx, size_t tx_len,
     uint8_t *rx, size_t rx_len)
{
  nrfx_spim_xfer_desc_t desc = NRFX_SPIM_XFER_TRX(tx, tx_len, rx, rx_len);
  nrfx_err_t err = nrfx_spim_xfer(inst, &desc, 0);

  if(err != NRFX_SUCCESS) {
    LOG_ERR("nrfx_spim_xfer failed: 0x%08lx\n", (unsigned long)err);
    return SPI_DEV_STATUS_TRANSFER_ERR;
  }

  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
/*
 * Slow path: stage the transfer through RAM buffers, a chunk at a time.
 * Used when the write buffer is not DMA-reachable, or when bytes have to be
 * clocked out and thrown away.
 */
static spi_status_t
transfer_staged(const nrfx_spim_t *inst,
                const uint8_t *write_buf, int wlen,
                uint8_t *inbuf, int rlen, int total)
{
  int done = 0;

  while(done < total) {
    int n = total - done;
    int i;
    spi_status_t status;

    if(n > NRF_SPI_CHUNK_SIZE) {
      n = NRF_SPI_CHUNK_SIZE;
    }

    for(i = 0; i < n; i++) {
      int idx = done + i;
      stage_tx[i] = idx < wlen ? write_buf[idx] : 0;
    }

    status = xfer(inst, stage_tx, n, stage_rx, n);
    if(status != SPI_DEV_STATUS_OK) {
      return status;
    }

    for(i = 0; i < n; i++) {
      int idx = done + i;
      if(idx < rlen) {
        inbuf[idx] = stage_rx[i];
      }
    }

    done += n;
  }

  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_arch_transfer(const spi_device_t *dev,
                  const uint8_t *write_buf, int wlen,
                  uint8_t *inbuf, int rlen,
                  int ignore_len)
{
  const nrfx_spim_t *inst;
  int total;

  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_NOT_OWNED;
  }

  if(write_buf == NULL && wlen > 0) {
    return SPI_DEV_STATUS_EINVAL;
  }
  if(inbuf == NULL && rlen > 0) {
    return SPI_DEV_STATUS_EINVAL;
  }
  if(wlen < 0 || rlen < 0 || ignore_len < 0) {
    return SPI_DEV_STATUS_EINVAL;
  }

  total = MAX(rlen + ignore_len, wlen);
  if(total == 0) {
    return SPI_DEV_STATUS_OK;
  }

  inst = &spim_instance[dev->spi_controller];

  /*
   * Fast path: nothing to discard, and both buffers are where EasyDMA can
   * reach them. nrfx clocks max(wlen, rlen) bytes, padding the shorter
   * side, which is exactly the contract of this function when
   * ignore_len is 0.
   */
  if(ignore_len == 0 &&
     (wlen == 0 || nrfx_is_in_ram(write_buf)) &&
     (rlen == 0 || nrfx_is_in_ram(inbuf))) {
    return xfer(inst, write_buf, wlen, inbuf, rlen);
  }

  return transfer_staged(inst, write_buf, wlen, inbuf, rlen, total);
}
/*---------------------------------------------------------------------------*/
#endif /* SPI_CONTROLLER_COUNT > 0 */
/*---------------------------------------------------------------------------*/
/**
 * @}
 */

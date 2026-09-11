/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
 * \file
 *         ENC28J60 SPI arch layer on top of the SPI HAL (os/dev/spi.h).
 *
 *         Unlike the per-board arch files, this one is platform independent:
 *         it works on any CPU that implements the SPI HAL, and a board only
 *         has to supply the pin defines below. Enable it by defining
 *         ENC28J60_CONF_USE_SPI_HAL as 1.
 *
 *         Note the cost model: the enc28j60 driver's arch interface moves one
 *         byte per call, so each byte here becomes its own SPI HAL transfer
 *         and, on nRF, its own EasyDMA transaction. That is the interface's
 *         shape, not something this layer can batch. It is why the practical
 *         limit on throughput is the driver rather than the bus clock.
 *
 *         The bus is acquired for the duration of a chip-select assertion
 *         rather than held forever, so the ENC28J60 can share a controller
 *         with other devices. The driver keeps CS asserted across a whole
 *         register access or packet transfer, so the *lock* is taken once per
 *         transaction, not per byte.
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#if ENC28J60_CONF_USE_SPI_HAL

#include "dev/spi.h"
#include "dev/gpio-hal.h"
#include "enc28j60.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "enc-spi"
/*
 * Errors on by default: a failed bus acquire in select() otherwise turns
 * into every register read returning zero with nothing to say why.
 */
#ifdef LOG_CONF_LEVEL_ENC28J60_SPI
#define LOG_LEVEL LOG_CONF_LEVEL_ENC28J60_SPI
#else
#define LOG_LEVEL LOG_LEVEL_ERR
#endif
/*---------------------------------------------------------------------------*/
#ifndef ETH_SPI_CONTROLLER
#define ETH_SPI_CONTROLLER 0
#endif

/*
 * The ENC28J60 is specified for up to 20 MHz. The SPI arch layer rounds a
 * request down to what the chosen controller instance can produce, so a
 * higher value here is capped rather than rejected. 8 MHz is what the
 * non-high-speed SPIM instances top out at anyway, and it is reliable on
 * jumper wires.
 */
#ifndef ETH_SPI_BIT_RATE
#define ETH_SPI_BIT_RATE 8000000
#endif

#if !defined(ETH_SPI_CLK_PORT) || !defined(ETH_SPI_CLK_PIN) || \
    !defined(ETH_SPI_MOSI_PORT) || !defined(ETH_SPI_MOSI_PIN) || \
    !defined(ETH_SPI_MISO_PORT) || !defined(ETH_SPI_MISO_PIN) || \
    !defined(ETH_SPI_CSN_PORT) || !defined(ETH_SPI_CSN_PIN)
#error "ENC28J60 SPI HAL arch: define ETH_SPI_{CLK,MOSI,MISO,CSN}_{PORT,PIN}"
#endif
/*---------------------------------------------------------------------------*/
/*
 * Set while the bus is held between select and deselect. A failed acquire
 * is logged once in select; the transfers that follow return 0 quietly
 * instead of logging once per byte for the rest of the transaction.
 */
static bool bus_held;
/*---------------------------------------------------------------------------*/
static const spi_device_t enc28j60_spi = {
  .port_spi_sck = ETH_SPI_CLK_PORT,
  .pin_spi_sck = ETH_SPI_CLK_PIN,
  .port_spi_mosi = ETH_SPI_MOSI_PORT,
  .pin_spi_mosi = ETH_SPI_MOSI_PIN,
  .port_spi_miso = ETH_SPI_MISO_PORT,
  .pin_spi_miso = ETH_SPI_MISO_PIN,
  .port_spi_cs = ETH_SPI_CSN_PORT,
  .pin_spi_cs = ETH_SPI_CSN_PIN,
  .spi_bit_rate = ETH_SPI_BIT_RATE,
  /* The ENC28J60 requires SPI mode 0. */
  .spi_pha = 0,
  .spi_pol = 0,
  .spi_controller = ETH_SPI_CONTROLLER,
};
/*---------------------------------------------------------------------------*/
void
enc28j60_arch_spi_init(void)
{
#if defined(ETH_RESET_PORT) && defined(ETH_RESET_PIN)
  /*
   * Drive RESET high (deasserted). The module has a pull-up, so leaving the
   * pin unconnected works too; driving it lets the board hold the chip in
   * reset if it ever needs to.
   */
  gpio_hal_arch_pin_set_output(ETH_RESET_PORT, ETH_RESET_PIN);
  gpio_hal_arch_set_pin(ETH_RESET_PORT, ETH_RESET_PIN);
#endif

  /*
   * Open and close the bus once so the pins are configured and a
   * misconfigured controller is reported here rather than at the first
   * register read.
   */
  if(spi_acquire(&enc28j60_spi) != SPI_DEV_STATUS_OK) {
    LOG_ERR("could not acquire the SPI bus\n");
    return;
  }
  spi_release(&enc28j60_spi);
}
/*---------------------------------------------------------------------------*/
void
enc28j60_arch_spi_select(void)
{
  if(spi_acquire(&enc28j60_spi) != SPI_DEV_STATUS_OK) {
    LOG_ERR("could not acquire the SPI bus\n");
    bus_held = false;
    return;
  }
  bus_held = true;
  spi_select(&enc28j60_spi);
}
/*---------------------------------------------------------------------------*/
void
enc28j60_arch_spi_deselect(void)
{
  if(!bus_held) {
    return;
  }
  spi_deselect(&enc28j60_spi);
  spi_release(&enc28j60_spi);
  bus_held = false;
}
/*---------------------------------------------------------------------------*/
uint8_t
enc28j60_arch_spi_write(uint8_t output)
{
  uint8_t input = 0;

  if(!bus_held) {
    return 0;
  }

  if(spi_transfer(&enc28j60_spi, &output, 1, &input, 1, 0)
     != SPI_DEV_STATUS_OK) {
    LOG_ERR("transfer failed\n");
    return 0;
  }

  return input;
}
/*---------------------------------------------------------------------------*/
uint8_t
enc28j60_arch_spi_read(void)
{
  return enc28j60_arch_spi_write(0);
}
/*---------------------------------------------------------------------------*/
#endif /* ENC28J60_CONF_USE_SPI_HAL */

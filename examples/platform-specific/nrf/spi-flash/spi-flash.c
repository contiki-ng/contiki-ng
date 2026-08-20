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
/**
 * \file
 *         Bring-up test for the nRF SPI (SPIM) driver.
 *
 *         Reads the JEDEC ID and the SFDP signature from the SPI NOR flash
 *         on the nRF54L15 DK (MX25R6435F on SPI00) and reports whether the
 *         values match what the part should return. Nothing is written, so
 *         this is safe to run against a flash holding data.
 *
 *         The point of testing against this device rather than a loopback
 *         is that it is already wired on the board: a wrong answer is the
 *         driver's fault, not the bench's.
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/spi.h"

#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Board wiring. Defaults are the nRF54L15 DK's onboard MX25R6435F. */
#ifndef SPI_FLASH_CONF_CONTROLLER
#define SPI_FLASH_CONTROLLER 0
#else
#define SPI_FLASH_CONTROLLER SPI_FLASH_CONF_CONTROLLER
#endif

#ifndef SPI_FLASH_CONF_SCK_PORT
#define SPI_FLASH_SCK_PORT   2
#define SPI_FLASH_SCK_PIN    1
#define SPI_FLASH_MOSI_PORT  2
#define SPI_FLASH_MOSI_PIN   2
#define SPI_FLASH_MISO_PORT  2
#define SPI_FLASH_MISO_PIN   4
#define SPI_FLASH_CS_PORT    2
#define SPI_FLASH_CS_PIN     5
#endif

/* The MX25R6435F is rated for 8 MHz in low-power mode. */
#ifndef SPI_FLASH_CONF_BIT_RATE
#define SPI_FLASH_BIT_RATE 8000000
#else
#define SPI_FLASH_BIT_RATE SPI_FLASH_CONF_BIT_RATE
#endif
/*---------------------------------------------------------------------------*/
/* Expected JEDEC ID of the MX25R6435F: Macronix, 0x28, 64 Mbit. */
#define EXPECTED_MANUFACTURER 0xc2
#define EXPECTED_TYPE         0x28
#define EXPECTED_CAPACITY     0x17
/*---------------------------------------------------------------------------*/
#define CMD_READ_JEDEC_ID 0x9f
#define CMD_READ_SFDP     0x5a
/*---------------------------------------------------------------------------*/
static const spi_device_t flash = {
  .port_spi_sck = SPI_FLASH_SCK_PORT,
  .pin_spi_sck = SPI_FLASH_SCK_PIN,
  .port_spi_mosi = SPI_FLASH_MOSI_PORT,
  .pin_spi_mosi = SPI_FLASH_MOSI_PIN,
  .port_spi_miso = SPI_FLASH_MISO_PORT,
  .pin_spi_miso = SPI_FLASH_MISO_PIN,
  .port_spi_cs = SPI_FLASH_CS_PORT,
  .pin_spi_cs = SPI_FLASH_CS_PIN,
  .spi_bit_rate = SPI_FLASH_BIT_RATE,
  .spi_pha = 0,
  .spi_pol = 0,
  .spi_controller = SPI_FLASH_CONTROLLER,
};
/*---------------------------------------------------------------------------*/
PROCESS(spi_flash_process, "nRF SPI flash test");
AUTOSTART_PROCESSES(&spi_flash_process);
/*---------------------------------------------------------------------------*/
static bool
read_jedec_id(uint8_t *id)
{
  const uint8_t cmd = CMD_READ_JEDEC_ID;

  if(spi_acquire(&flash) != SPI_DEV_STATUS_OK) {
    printf("FAIL: could not acquire the SPI bus\n");
    return false;
  }

  spi_select(&flash);
  /* Write the opcode, then clock in three ID bytes. */
  if(spi_transfer(&flash, &cmd, 1, NULL, 0, 0) != SPI_DEV_STATUS_OK ||
     spi_transfer(&flash, NULL, 0, id, 3, 0) != SPI_DEV_STATUS_OK) {
    spi_deselect(&flash);
    spi_release(&flash);
    printf("FAIL: JEDEC ID transfer error\n");
    return false;
  }
  spi_deselect(&flash);

  spi_release(&flash);
  return true;
}
/*---------------------------------------------------------------------------*/
/*
 * Repeats the JEDEC ID read with the opcode in a flash-resident buffer.
 *
 * EasyDMA can only reach Data RAM, so a write buffer in flash must be staged
 * through a RAM bounce buffer by the driver. This is the failure mode that
 * silently corrupts transfers rather than erroring, so it is worth an explicit
 * test: the answer must match the RAM-buffer read byte for byte.
 */
static bool
read_jedec_id_from_flash_buf(uint8_t *id)
{
  /* static const lands in .rodata, i.e. flash, not RAM. */
  static const uint8_t cmd = CMD_READ_JEDEC_ID;

  if(spi_acquire(&flash) != SPI_DEV_STATUS_OK) {
    printf("FAIL: could not acquire the SPI bus\n");
    return false;
  }

  spi_select(&flash);
  if(spi_transfer(&flash, &cmd, 1, NULL, 0, 0) != SPI_DEV_STATUS_OK ||
     spi_transfer(&flash, NULL, 0, id, 3, 0) != SPI_DEV_STATUS_OK) {
    spi_deselect(&flash);
    spi_release(&flash);
    printf("FAIL: flash-buffer JEDEC ID transfer error\n");
    return false;
  }
  spi_deselect(&flash);

  spi_release(&flash);
  return true;
}
/*---------------------------------------------------------------------------*/
/*
 * Reads the SFDP signature at address 0.
 *
 * The opcode and 3 address bytes are followed by one dummy byte before data
 * starts, so the dummy is appended to the write phase: ignore_len in the SPI
 * HAL discards trailing bytes, not leading ones.
 *
 * The read then asks for 8 bytes but keeps only the first 4, which exercises
 * the ignore_len path and, with it, the staged (bounce-buffered) transfer.
 */
static bool
read_sfdp_signature(uint8_t *sig)
{
  const uint8_t cmd[5] = { CMD_READ_SFDP, 0x00, 0x00, 0x00, 0x00 };

  if(spi_acquire(&flash) != SPI_DEV_STATUS_OK) {
    printf("FAIL: could not acquire the SPI bus\n");
    return false;
  }

  spi_select(&flash);
  if(spi_transfer(&flash, cmd, sizeof(cmd), NULL, 0, 0) != SPI_DEV_STATUS_OK ||
     spi_transfer(&flash, NULL, 0, sig, 4, 4) != SPI_DEV_STATUS_OK) {
    spi_deselect(&flash);
    spi_release(&flash);
    printf("FAIL: SFDP transfer error\n");
    return false;
  }
  spi_deselect(&flash);

  spi_release(&flash);
  return true;
}
/*---------------------------------------------------------------------------*/
/*
 * Probes which bit rates the controller accepts, and exercises a staged
 * transfer longer than NRF_SPI_CHUNK_SIZE so the chunking loop is covered.
 *
 * nrfx rejects a bit rate its prescaler cannot produce
 * (NRFX_ERROR_INVALID_PARAM), which surfaces here as a failed open rather
 * than a silently different clock -- so the accepted set is worth knowing
 * before wiring a device with a maximum rate of its own.
 */
static void
probe_bit_rates(void)
{
  static const uint32_t rates[] = {
    1000000, 2000000, 4000000, 8000000, 16000000, 20000000, 32000000
  };
  unsigned i;

  printf("  bit rates:");
  for(i = 0; i < sizeof(rates) / sizeof(rates[0]); i++) {
    spi_device_t probe = flash;
    probe.spi_bit_rate = rates[i];

    if(spi_acquire(&probe) == SPI_DEV_STATUS_OK) {
      uint8_t id[3];
      const uint8_t cmd = CMD_READ_JEDEC_ID;
      bool ok;

      spi_select(&probe);
      ok = spi_transfer(&probe, &cmd, 1, NULL, 0, 0) == SPI_DEV_STATUS_OK &&
           spi_transfer(&probe, NULL, 0, id, 3, 0) == SPI_DEV_STATUS_OK &&
           id[0] == EXPECTED_MANUFACTURER;
      spi_deselect(&probe);
      spi_release(&probe);

      printf(" %lu:%s", (unsigned long)(rates[i] / 1000000),
             ok ? "ok" : "bad");
    } else {
      printf(" %lu:rej", (unsigned long)(rates[i] / 1000000));
    }
  }
  printf(" (MHz)\n");
}
/*---------------------------------------------------------------------------*/
/*
 * Reads a whole SFDP table with a discard tail, so the staged path has to
 * loop over several chunks rather than fitting in one.
 */
static bool
read_sfdp_long(void)
{
  const uint8_t cmd[5] = { CMD_READ_SFDP, 0x00, 0x00, 0x00, 0x00 };
  static uint8_t buf[200];
  bool ok;

  if(spi_acquire(&flash) != SPI_DEV_STATUS_OK) {
    return false;
  }

  spi_select(&flash);
  /* 200 kept + 56 discarded = 256 bytes clocked, well over the chunk size. */
  ok = spi_transfer(&flash, cmd, sizeof(cmd), NULL, 0, 0) == SPI_DEV_STATUS_OK &&
       spi_transfer(&flash, NULL, 0, buf, sizeof(buf), 56) == SPI_DEV_STATUS_OK;
  spi_deselect(&flash);
  spi_release(&flash);

  if(!ok) {
    printf("  long SFDP: transfer error\n");
    return false;
  }

  printf("  long SFDP: %02x %02x %02x %02x ... %02x %02x",
         buf[0], buf[1], buf[2], buf[3], buf[198], buf[199]);
  if(buf[0] == 'S' && buf[1] == 'F' && buf[2] == 'D' && buf[3] == 'P') {
    printf("  OK (%u B staged)\n", (unsigned)(sizeof(buf) + 56));
    return true;
  }
  printf("  MISMATCH\n");
  return false;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(spi_flash_process, ev, data)
{
  static uint8_t id[3];
  static uint8_t id2[3];
  static uint8_t sig[4];
  static int failures;
  static struct etimer periodic;

  PROCESS_BEGIN();

  /*
   * The report is repeated rather than printed once at boot, so that it
   * survives a debugger-triggered reset and so a terminal can be attached
   * at any time. It also means wiring can be changed and the result
   * re-read without reflashing.
   */
  while(1) {

  failures = 0;

  /* Report the configuration rather than leaving it to be inferred. */
  printf("\nnRF SPI flash test\n");
  printf("  controller %u, %lu Hz, mode %u%u\n",
         (unsigned)flash.spi_controller,
         (unsigned long)flash.spi_bit_rate,
         (unsigned)flash.spi_pol, (unsigned)flash.spi_pha);
  printf("  SCK P%u.%02u  MOSI P%u.%02u  MISO P%u.%02u  CS P%u.%02u\n",
         SPI_FLASH_SCK_PORT, SPI_FLASH_SCK_PIN,
         SPI_FLASH_MOSI_PORT, SPI_FLASH_MOSI_PIN,
         SPI_FLASH_MISO_PORT, SPI_FLASH_MISO_PIN,
         SPI_FLASH_CS_PORT, SPI_FLASH_CS_PIN);

  if(read_jedec_id(id)) {
    printf("  JEDEC ID: %02x %02x %02x", id[0], id[1], id[2]);
    if(id[0] == EXPECTED_MANUFACTURER && id[1] == EXPECTED_TYPE &&
       id[2] == EXPECTED_CAPACITY) {
      printf("  OK (MX25R6435F)\n");
    } else {
      printf("  MISMATCH (expected %02x %02x %02x)\n",
             EXPECTED_MANUFACTURER, EXPECTED_TYPE, EXPECTED_CAPACITY);
      if((id[0] == 0x00 && id[1] == 0x00 && id[2] == 0x00) ||
         (id[0] == 0xff && id[1] == 0xff && id[2] == 0xff)) {
        printf("  all-%s usually means MISO is not connected, CS is wrong,\n"
               "  or the flash is not powered\n",
               id[0] == 0 ? "zeroes" : "ones");
      }
      failures++;
    }
  } else {
    failures++;
  }

  if(read_jedec_id_from_flash_buf(id2)) {
    printf("  flash-buf: %02x %02x %02x", id2[0], id2[1], id2[2]);
    if(id2[0] == id[0] && id2[1] == id[1] && id2[2] == id[2]) {
      printf("  OK (DMA staging)\n");
    } else {
      printf("  MISMATCH (RAM buffer gave %02x %02x %02x)\n",
             id[0], id[1], id[2]);
      failures++;
    }
  } else {
    failures++;
  }

  if(read_sfdp_signature(sig)) {
    printf("  SFDP:     %02x %02x %02x %02x", sig[0], sig[1], sig[2], sig[3]);
    /* "SFDP" in ASCII. */
    if(sig[0] == 'S' && sig[1] == 'F' && sig[2] == 'D' && sig[3] == 'P') {
      printf("  OK\n");
    } else {
      printf("  MISMATCH (expected 53 46 44 50)\n");
      failures++;
    }
  } else {
    failures++;
  }

  if(!read_sfdp_long()) {
    failures++;
  }

  probe_bit_rates();

  printf("%s\n", failures == 0 ? "SPI OK" : "SPI FAILED");

  etimer_set(&periodic, CLOCK_SECOND * 5);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

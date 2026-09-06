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
/**
 * \file
 *         Bring-up test for an ENC28J60 Ethernet module.
 *
 *         Probes the chip over SPI before handing it to the driver: soft
 *         reset, wait for the oscillator, read the revision ID, then read
 *         back a register that was just written. Only if all of that passes
 *         does it call enc28j60_init(), which spins forever waiting for
 *         ESTAT.CLKRDY if the chip is not answering.
 *
 *         Once initialised it reports every frame the chip receives, so a
 *         cable into a live switch is enough to show the RX path working
 *         end to end.
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "enc28j60.h"

#include <stdio.h>
/*---------------------------------------------------------------------------*/
/* SPI opcodes, section 4.2 of the datasheet. */
#define CMD_RCR 0x00 /* Read Control Register */
#define CMD_WCR 0x40 /* Write Control Register */
#define CMD_BFS 0x80 /* Bit Field Set */
#define CMD_BFC 0xa0 /* Bit Field Clear */
#define CMD_SRC 0xff /* System Reset (soft reset) */

/* Registers that are mapped into every bank. */
#define ESTAT   0x1d
#define ECON1   0x1f

#define ESTAT_CLKRDY 0x01
#define ECON1_BSEL   0x03

/* Bank 3. */
#define EREVID  0x12
/*
 * MAC address registers, also bank 3. Unlike EREVID these are MAC registers,
 * so a read clocks out a dummy byte before the data (datasheet 4.2.1). That
 * makes them the only part of this test that exercises the dummy-byte path.
 */
#define MAADR1  0x04
#define MAADR2  0x05
#define MAADR3  0x02
#define MAADR4  0x03
#define MAADR5  0x00
#define MAADR6  0x01
/* Bank 0, and writable, so it doubles as a register read/write check. */
#define EWRPTL  0x02
/*---------------------------------------------------------------------------*/
static uint8_t
readreg(uint8_t reg)
{
  uint8_t r;

  enc28j60_arch_spi_select();
  enc28j60_arch_spi_write(CMD_RCR | (reg & 0x1f));
  /* EREVID, ESTAT, ECON1 and EWRPTL are all ETH registers: no dummy byte. */
  r = enc28j60_arch_spi_read();
  enc28j60_arch_spi_deselect();

  return r;
}
/*---------------------------------------------------------------------------*/
static uint8_t
readreg_mac(uint8_t reg)
{
  uint8_t r;

  enc28j60_arch_spi_select();
  enc28j60_arch_spi_write(CMD_RCR | (reg & 0x1f));
  /* MAC and MII registers return a dummy byte first. */
  enc28j60_arch_spi_read();
  r = enc28j60_arch_spi_read();
  enc28j60_arch_spi_deselect();

  return r;
}
/*---------------------------------------------------------------------------*/
static void
writereg(uint8_t reg, uint8_t data)
{
  enc28j60_arch_spi_select();
  enc28j60_arch_spi_write(CMD_WCR | (reg & 0x1f));
  enc28j60_arch_spi_write(data);
  enc28j60_arch_spi_deselect();
}
/*---------------------------------------------------------------------------*/
static void
setbank(uint8_t bank)
{
  enc28j60_arch_spi_select();
  enc28j60_arch_spi_write(CMD_BFC | ECON1);
  enc28j60_arch_spi_write(ECON1_BSEL);
  enc28j60_arch_spi_deselect();

  enc28j60_arch_spi_select();
  enc28j60_arch_spi_write(CMD_BFS | ECON1);
  enc28j60_arch_spi_write(bank & ECON1_BSEL);
  enc28j60_arch_spi_deselect();
}
/*---------------------------------------------------------------------------*/
static void
softreset(void)
{
  enc28j60_arch_spi_select();
  enc28j60_arch_spi_write(CMD_SRC);
  enc28j60_arch_spi_deselect();
}
/*---------------------------------------------------------------------------*/
static void
explain(uint8_t v)
{
  if(v == 0x00) {
    printf("  all zeroes: MISO is stuck low. Check MISO, and that the\n"
           "  module has 3.3 V on VCC and a common ground.\n");
  } else if(v == 0xff) {
    printf("  all ones: nothing is driving MISO. Check MISO and CS, and\n"
           "  that SI/SO are not swapped (SI takes MOSI, SO feeds MISO).\n");
  } else {
    printf("  unexpected value: check SCK, and try a lower ETH_SPI_BIT_RATE.\n");
  }
}
/*---------------------------------------------------------------------------*/
/*
 * Returns non-zero if the chip answers plausibly. Deliberately does not use
 * anything from enc28j60.c, so a failure here is about the wiring and the SPI
 * driver rather than about the Ethernet driver.
 */
static int
probe(void)
{
  uint8_t rev;
  uint8_t estat;
  uint8_t readback;
  int i;

  enc28j60_arch_spi_init();

  softreset();
  /* Errata #2: wait at least 1 ms after a soft reset before any access. */
  clock_delay_usec(2000);

  /* The oscillator start-up is specified as 300 us; allow far more. */
  estat = 0;
  for(i = 0; i < 100; i++) {
    estat = readreg(ESTAT);
    if(estat != 0x00 && estat != 0xff && (estat & ESTAT_CLKRDY)) {
      break;
    }
    clock_delay_usec(1000);
  }

  printf("  ESTAT:    %02x", estat);
  if(estat != 0x00 && estat != 0xff && (estat & ESTAT_CLKRDY)) {
    printf("  OK (CLKRDY set)\n");
  } else {
    printf("  FAIL (no CLKRDY)\n");
    explain(estat);
    return 0;
  }

  setbank(3);
  rev = readreg(EREVID);
  printf("  EREVID:   %02x", rev);
  if(rev != 0x00 && rev != 0xff) {
    printf("  OK (silicon rev B%u)\n", rev == 2 ? 1 : (rev == 6 ? 7 : rev));
  } else {
    printf("  FAIL\n");
    explain(rev);
    return 0;
  }

  /*
   * Write and read back a scratch register. The revision ID alone can be
   * right by luck on a marginal bus; a value we chose cannot.
   */
  setbank(0);
  writereg(EWRPTL, 0x5a);
  readback = readreg(EWRPTL);
  printf("  reg r/w:  %02x", readback);
  if(readback == 0x5a) {
    printf("  OK\n");
  } else {
    printf("  FAIL (wrote 5a)\n");
    explain(readback);
    return 0;
  }

  /*
   * Write a MAC address and read it back. The ENC28J60 has no factory MAC of
   * its own -- these registers come up undefined and the host is expected to
   * supply one -- so this is a round-trip check, not an identity read. Its
   * value is that MAC registers need the dummy byte that ETH registers do not,
   * so it covers an access pattern nothing above does, across six registers
   * whose addresses are deliberately not in order.
   *
   * enc28j60_init() soft-resets the chip and writes its own MAC afterwards,
   * so nothing here is left behind.
   */
  {
    static const uint8_t probe_mac[6] = {
      0x02, 0xde, 0xad, 0xbe, 0xef, 0x01
    };
    static const uint8_t maadr[6] = {
      MAADR1, MAADR2, MAADR3, MAADR4, MAADR5, MAADR6
    };
    uint8_t got[6];
    int ok = 1;

    setbank(3);
    for(i = 0; i < 6; i++) {
      writereg(maadr[i], probe_mac[i]);
    }
    for(i = 0; i < 6; i++) {
      got[i] = readreg_mac(maadr[i]);
      if(got[i] != probe_mac[i]) {
        ok = 0;
      }
    }

    printf("  MAC r/w:  %02x:%02x:%02x:%02x:%02x:%02x",
           got[0], got[1], got[2], got[3], got[4], got[5]);
    if(ok) {
      printf("  OK\n");
    } else {
      printf("  FAIL (wrote %02x:%02x:%02x:%02x:%02x:%02x)\n",
             probe_mac[0], probe_mac[1], probe_mac[2],
             probe_mac[3], probe_mac[4], probe_mac[5]);
      printf("  a one-byte shift here usually means the dummy byte on MAC\n"
             "  register reads is being mishandled\n");
      return 0;
    }
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS(enc28j60_test_process, "ENC28J60 test");
AUTOSTART_PROCESSES(&enc28j60_test_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(enc28j60_test_process, ev, data)
{
  static uint8_t mac[6] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };
  static uint8_t buf[1518];
  static struct etimer periodic;
  static unsigned long frames;
  static unsigned ticks;
  static int ok;

  PROCESS_BEGIN();

  printf("\nENC28J60 test\n");
  printf("  controller %u, %u Hz\n",
         (unsigned)ETH_SPI_CONTROLLER, (unsigned)ETH_SPI_BIT_RATE);
  printf("  SCK P%u.%02u  MOSI P%u.%02u  MISO P%u.%02u  CS P%u.%02u\n",
         ETH_SPI_CLK_PORT, ETH_SPI_CLK_PIN,
         ETH_SPI_MOSI_PORT, ETH_SPI_MOSI_PIN,
         ETH_SPI_MISO_PORT, ETH_SPI_MISO_PIN,
         ETH_SPI_CSN_PORT, ETH_SPI_CSN_PIN);

  ok = probe();
  if(!ok) {
    printf("ENC28J60 NOT FOUND\n");
    PROCESS_EXIT();
  }
  printf("ENC28J60 OK\n");

  /*
   * Safe to call now: enc28j60_init() waits for ESTAT.CLKRDY in a loop with
   * no timeout, which the probe above has already shown will be satisfied.
   */
  printf("initialising driver, MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  enc28j60_init(mac);

  printf("polling for frames -- plug the module into a switch\n");
  frames = 0;
  ticks = 0;
  while(1) {
    int len;

    etimer_set(&periodic, CLOCK_SECOND / 8);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic));

    len = enc28j60_read(buf, sizeof(buf));
    if(len > 0) {
      frames++;
      printf("rx %lu: %d bytes, dst %02x:%02x:%02x:%02x:%02x:%02x "
             "type %02x%02x\n", frames, len,
             buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
             buf[12], buf[13]);
    }

    /*
     * Heartbeat every ~5 s. Without it the firmware is silent until a frame
     * arrives, which is indistinguishable from a crash or a dead capture
     * path -- and it means the state can be read at any time without
     * resetting the board to catch a one-shot banner.
     */
    if(++ticks >= 40) {
      ticks = 0;
      printf("alive: %lu frames so far%s\n", frames,
             frames == 0 ? " (no cable, or nothing on the link yet)" : "");
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

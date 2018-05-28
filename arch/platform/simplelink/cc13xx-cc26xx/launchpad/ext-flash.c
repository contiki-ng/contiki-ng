/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
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
 * \addtogroup sensortag-cc26xx-ext-flash
 * @{
 *
 * \file
 * Sensortag/LaunchPad External Flash Driver
 */
/*---------------------------------------------------------------------------*/
#include <Board.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "ext-flash.h"
#include "ti-lib.h"
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
static SPI_Handle spiHandle = NULL;
/*---------------------------------------------------------------------------*/
#define SPI_BIT_RATE              4000000

/* Instruction codes */
#define BLS_CODE_PROGRAM          0x02  /**< Page Program */
#define BLS_CODE_READ             0x03  /**< Read Data */
#define BLS_CODE_READ_STATUS      0x05  /**< Read Status Register */
#define BLS_CODE_WRITE_ENABLE     0x06  /**< Write Enable */
#define BLS_CODE_SECTOR_ERASE     0x20  /**< Sector Erase */
#define BLS_CODE_MDID             0x90  /**< Manufacturer Device ID */

#define BLS_CODE_DP               0xB9  /**< Power down */
#define BLS_CODE_RDP              0xAB  /**< Power standby */

/* Erase instructions */
#define BLS_CODE_ERASE_4K         0x20  /**< Sector Erase */
#define BLS_CODE_ERASE_32K        0x52
#define BLS_CODE_ERASE_64K        0xD8
#define BLS_CODE_ERASE_ALL        0xC7  /**< Mass Erase */

/* Bitmasks of the status register */
#define BLS_STATUS_SRWD_BM        0x80
#define BLS_STATUS_BP_BM          0x0C
#define BLS_STATUS_WEL_BM         0x02
#define BLS_STATUS_WIP_BM         0x01

#define BLS_STATUS_BIT_BUSY       0x01  /**< Busy bit of the status register */

/* Part specific constants */
#define BLS_PROGRAM_PAGE_SIZE     0x100     /**< Page size 0x100 */
#define BLS_ERASE_SECTOR_SIZE     0x1000    /**< Sector size 0x1000 */
/*---------------------------------------------------------------------------*/
typedef struct
{
    uint8_t manfId;             /**< Manufacturer ID */
    uint8_t devId;              /**< Device ID */
} ExtFlashInfo;
/* Supported flash devices */
static const ExtFlashInfo supported_devices[] =
{
    {
        .manfId = 0xC2,         /**< Macronics MX25R1635F */
        .devId  = 0x15
    },
    {
        .manfId = 0xC2,         /**< Macronics MX25R8035F */
        .devId  = 0x14
    },
    {
        .manfId = 0xEF,         /**< WinBond W25X40CL */
        .devId  = 0x12
    },
    {
        .manfId = 0xEF,         /**< WinBond W25X20CL */
        .devId  = 0x11
    }
};
/*---------------------------------------------------------------------------*/
static bool
spi_write(const uint8_t *buf, size_t len)
{
  SPI_Transaction spiTransaction;
  spiTransaction.count = len;
  spiTransaction.txBuf = (void *)buf;
  spiTransaction.rxBuf = NULL;

  return SPI_transfer(spiHandle, &spiTransaction);
}
/*---------------------------------------------------------------------------*/
static bool
spi_read(uint8_t *buf, size_t len)
{
  SPI_Transaction spiTransaction;
  spiTransaction.count = len;
  spiTransaction.txBuf = NULL;
  spiTransaction.rxBuf = buf;

  return SPI_transfer(spiHandle, &spiTransaction);
}
/*---------------------------------------------------------------------------*/
static void
select(void)
{
    ti_lib_gpio_clear_dio(Board_SPI_FLASH_CS);
}
/*---------------------------------------------------------------------------*/
static void
deselect(void)
{
    ti_lib_gpio_set_dio(Board_SPI_FLASH_CS);
}
/*---------------------------------------------------------------------------*/
static bool
wait_ready(void)
{
  const uint8_t wbuf[] = { BLS_CODE_READ_STATUS };
  uint8_t rbuf[1] = { 0x0 };

  /* TODO are 1000 tries enough? */
  for (size_t i = 0; i < 1000; ++i) {
    select();

    const bool spi_ok = spi_write(wbuf, sizeof(wbuf))
                     && spi_read(rbuf, sizeof(rbuf));

    deselect();

    if (!spi_ok) {
        /* Error */
        return false;
    }
    if (!(rbuf[0] & BLS_STATUS_BIT_BUSY)) {
        /* Now ready */
        return true;
    }
  }

  return false;
}
/*---------------------------------------------------------------------------*/
static bool
power_standby(void)
{
  const uint8_t cmd[] = { BLS_CODE_RDP };

  select();

  const bool spi_ok = spi_write(cmd, sizeof(cmd));

  deselect();

  if (!spi_ok) {
    return false;
  }

  /* Waking up of the device is manufacturer dependent.
  * for a Winond chip-set, once the request to wake up the flash has been
  * send, CS needs to stay high at least 3us (for Winbond part)
  * for chip-set like Macronix, it can take up to 35us.
  * 3 cycles per loop: 560 loops @ 48 MHz = 35 us */
  ti_lib_cpu_delay(560);

  return wait_ready();
}
/*---------------------------------------------------------------------------*/
static bool
power_down(void)
{
  const uint8_t cmd[] = { BLS_CODE_DP };

  select();

  const bool spi_ok = spi_write(cmd, sizeof(cmd));

  deselect();

  return spi_ok;
}
/*---------------------------------------------------------------------------*/
static bool
write_enable(void)
{
  const uint8_t wbuf[] = { BLS_CODE_WRITE_ENABLE };

  select();

  const bool spi_ok = spi_write(wbuf, sizeof(wbuf));

  deselect();

  return spi_ok;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Verify the flash part.
 * \retval bool true on success; else, false
 */
static bool
verify_part(void)
{
  const uint8_t wbuf[] = { BLS_CODE_MDID, 0xFF, 0xFF, 0x00 };
  uint8_t rbuf[2] = { 0x0 };

  const bool spi_ok = spi_write(wbuf, sizeof(wbuf))
                   && spi_read(rbuf, sizeof(rbuf));

  if (!spi_ok) {
    return false;
  }

  const ExtFlashInfo curr_device = {
    .manfId = rbuf[0],
    .devId  = rbuf[1]
  };

  const size_t num_devices = sizeof(supported_devices) / sizeof(supported_devices[0]);
  for (size_t i = 0; i < num_devices; ++i) {
    if (curr_device.manfId == supported_devices[i].manfId &&
        curr_device.devId == supported_devices[i].devId) {
      return true;
    }
  }

  return false;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_open()
{
  if (spiHandle != NULL) {
    return false;
  }

  SPI_Params spiParams;
  SPI_Params_init(&spiParams);
  spiParams.bitRate = SPI_BIT_RATE;
  spiParams.mode = SPI_MASTER;
  spiParams.transferMode = SPI_MODE_BLOCKING;

  spiHandle = SPI_open(Board_SPI0, &spiParams);

  if (spiHandle == NULL) {
    return false;
  }

  ti_lib_ioc_pin_type_gpio_output(Board_SPI_FLASH_CS);
  deselect();

  const bool is_powered = power_standby();
  if (!is_powered) {
    ext_flash_close();
    return false;
  }

  return verify_part();
}
/*---------------------------------------------------------------------------*/
void
ext_flash_close()
{
  if (spiHandle == NULL) {
    return;
  }

  power_down();
  SPI_close(spiHandle);
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_read(size_t offset, size_t length, uint8_t *buf)
{
  if (spiHandle == NULL || buf == NULL) {
    return false;
  }

  if (length == 0) {
    return true;
  }

  const bool is_ready = wait_ready();
  if (!is_ready) {
    return false;
  }

  const uint8_t wbuf[] = {
    BLS_CODE_READ,
    (offset >> 16) & 0xFF,
    (offset >>  8) & 0xFF,
    (offset >>  0) & 0xFF,
  };

  select();

  const bool spi_ok = spi_write(wbuf, sizeof(wbuf))
                   && spi_read(buf, length);

  deselect();

  return (spi_ok);
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_write(size_t offset, size_t length, const uint8_t *buf)
{
  if (spiHandle == NULL || buf == NULL) {
    return false;
  }

  uint8_t wbuf[4] = { BLS_CODE_PROGRAM, 0, 0, 0 };

  while (length > 0)
  {
    /* Wait till previous erase/program operation completes */
    if (!wait_ready()) {
      return false;
    }

    /* Enable writing */
    if (!write_enable()) {
        return false;
    }

    /* Interim length per instruction */
    size_t ilen = BLS_PROGRAM_PAGE_SIZE - (offset % BLS_PROGRAM_PAGE_SIZE);
    if (length < ilen) {
      ilen = length;
    }

    wbuf[1] = (offset >> 16) & 0xFF;
    wbuf[2] = (offset >>  8) & 0xFF;
    wbuf[3] = (offset >>  0) & 0xFF;

    offset += ilen;
    length -= ilen;

    /* Up to 100ns CS hold time (which is not clear
    * whether it's application only in between reads)
    * is not imposed here since above instructions
    * should be enough to delay
    * as much. */
    select();

    const bool spi_ok = spi_write(wbuf, sizeof(wbuf))
                     && spi_write(buf, ilen);

    buf += ilen;

    deselect();

    if (!spi_ok) {
      return false;
    }
  }

  return true;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_erase(size_t offset, size_t length)
{
  /* Note that Block erase might be more efficient when the floor map
  * is well planned for OTA but to simplify for the temporary implemetation,
  * sector erase is used blindly. */
  uint8_t wbuf[4] = { BLS_CODE_SECTOR_ERASE, 0x0, 0x0, 0x0 };

  const size_t endoffset = offset + length - 1;
  offset = (offset / BLS_ERASE_SECTOR_SIZE) * BLS_ERASE_SECTOR_SIZE;
  const size_t numsectors = (endoffset - offset + BLS_ERASE_SECTOR_SIZE - 1) / BLS_ERASE_SECTOR_SIZE;

  for (size_t i = 0; i < numsectors; ++i) {
    /* Wait till previous erase/program operation completes */
    if (!wait_ready()) {
      return false;
    }

    /* Enable writing */
    if (!write_enable()) {
        return false;
    }

    wbuf[1] = (offset >> 16) & 0xFF;
    wbuf[2] = (offset >>  8) & 0xFF;
    wbuf[3] = (offset >>  0) & 0xFF;

    select();

    const bool spi_ok = spi_write(wbuf, sizeof(wbuf));

    deselect();

    if (!spi_ok) {
      return false;
    }

    offset += BLS_ERASE_SECTOR_SIZE;
  }

  return true;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_test(void)
{
  const bool ret = ext_flash_open();
  ext_flash_close();

  return ret;
}
/*---------------------------------------------------------------------------*/
void
ext_flash_init()
{
  GPIO_init();
  SPI_init();
}
/*---------------------------------------------------------------------------*/
/** @} */

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
#include "contiki.h"
#include "ext-flash.h"
#include "dev/spi.h"
#include "gpio-hal.h"
#include "sys/log.h"

#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#ifndef EXT_FLASH_SPI_CONTROLLER

#define EXT_FLASH_SPI_CONTROLLER      0xFF /* No controller */

#define EXT_FLASH_SPI_PIN_SCK         GPIO_HAL_PIN_UNKNOWN
#define EXT_FLASH_SPI_PIN_MOSI        GPIO_HAL_PIN_UNKNOWN
#define EXT_FLASH_SPI_PIN_MISO        GPIO_HAL_PIN_UNKNOWN
#define EXT_FLASH_SPI_PIN_CS          GPIO_HAL_PIN_UNKNOWN

#define EXT_FLASH_DEVICE_ID           0xFF
#define EXT_FLASH_MID                 0xFF

#define EXT_FLASH_PROGRAM_PAGE_SIZE   256
#define EXT_FLASH_ERASE_SECTOR_SIZE   4096

#endif /* EXT_FLASH_SPI_CONTROLLER */
/*---------------------------------------------------------------------------*/
/* Log configuration */
#define LOG_MODULE "ext-flash"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
/* Instruction codes */

#define BLS_CODE_PROGRAM          0x02 /**< Page Program */
#define BLS_CODE_READ             0x03 /**< Read Data */
#define BLS_CODE_READ_STATUS      0x05 /**< Read Status Register */
#define BLS_CODE_WRITE_ENABLE     0x06 /**< Write Enable */
#define BLS_CODE_SECTOR_ERASE     0x20 /**< Sector Erase */
#define BLS_CODE_MDID             0x90 /**< Manufacturer Device ID */

#define BLS_CODE_PD               0xB9 /**< Power down */
#define BLS_CODE_RPD              0xAB /**< Release Power-Down */
/*---------------------------------------------------------------------------*/
/* Erase instructions */

#define BLS_CODE_ERASE_4K         0x20 /**< Sector Erase */
#define BLS_CODE_ERASE_32K        0x52
#define BLS_CODE_ERASE_64K        0xD8
#define BLS_CODE_ERASE_ALL        0xC7 /**< Mass Erase */
/*---------------------------------------------------------------------------*/
/* Bitmasks of the status register */

#define BLS_STATUS_SRWD_BM        0x80
#define BLS_STATUS_BP_BM          0x0C
#define BLS_STATUS_WEL_BM         0x02
#define BLS_STATUS_WIP_BM         0x01

#define BLS_STATUS_BIT_BUSY       0x01 /**< Busy bit of the status register */
/*---------------------------------------------------------------------------*/
#define VERIFY_PART_LOCKED          -2
#define VERIFY_PART_ERROR           -1
#define VERIFY_PART_POWERED_DOWN     0
#define VERIFY_PART_OK               1
/*---------------------------------------------------------------------------*/
static spi_device_t flash_spi_configuration_default = {
  .spi_controller = EXT_FLASH_SPI_CONTROLLER,
  .pin_spi_sck = EXT_FLASH_SPI_PIN_SCK,
  .pin_spi_miso = EXT_FLASH_SPI_PIN_MISO,
  .pin_spi_mosi = EXT_FLASH_SPI_PIN_MOSI,
  .pin_spi_cs = EXT_FLASH_SPI_PIN_CS,
  .spi_bit_rate = 4000000,
  .spi_pha = 0,
  .spi_pol = 0
};

static spi_device_t *flash_spi_configuration;
/**
 * Clear external flash CSN line
 */
static bool
select_on_bus(void)
{
  if(spi_select(flash_spi_configuration) == SPI_DEV_STATUS_OK) {
    return true;
  }
  return false;
}
/*---------------------------------------------------------------------------*/
/**
 * Set external flash CSN line
 */
static void
deselect(void)
{
  spi_deselect(flash_spi_configuration);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Wait till previous erase/program operation completes.
 * \return True when successful.
 */
static bool
wait_ready(void)
{
  bool ret;
  const uint8_t wbuf[1] = { BLS_CODE_READ_STATUS };

  if(select_on_bus() == false) {
    return false;
  }

  ret = spi_write(flash_spi_configuration, wbuf, sizeof(wbuf));

  if(ret != SPI_DEV_STATUS_OK) {
    deselect();
    return false;
  }

  for(;;) {
    uint8_t buf;
    /* Note that this temporary implementation is not
     * energy efficient.
     * Thread could have yielded while waiting for flash
     * erase/program to complete.
     */
    ret = spi_read(flash_spi_configuration, &buf, sizeof(buf));

    if(ret != SPI_DEV_STATUS_OK) {
      /* Error */
      deselect();
      return false;
    }

    if(!(buf & BLS_STATUS_BIT_BUSY)) {
      /* Now ready */
      break;
    }
  }
  deselect();
  return true;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Verify the flash part.
 * \retval VERIFY_PART_OK The part was identified successfully
 * \retval VERIFY_PART_ERROR There was an error communicating with the part
 * \retval VERIFY_PART_POWERED_DOWN Communication was successful, but the part
 *         was powered down
 */
static uint8_t
verify_part(void)
{
  const uint8_t wbuf[] = { BLS_CODE_MDID, 0xFF, 0xFF, 0x00 };
  uint8_t rbuf[2] = { 0, 0 };
  bool ret;

  if(select_on_bus() == false) {
    return VERIFY_PART_LOCKED;
  }

  if(spi_write(flash_spi_configuration, wbuf, sizeof(wbuf)) != SPI_DEV_STATUS_OK) {
    deselect();
    return VERIFY_PART_ERROR;
  }

  ret = spi_read(flash_spi_configuration, rbuf, sizeof(rbuf));
  deselect();
  if(ret != SPI_DEV_STATUS_OK) {
    return VERIFY_PART_ERROR;
  }

  LOG_DBG("Verify: %02x %02x\n", rbuf[0], rbuf[1]);

  if(rbuf[0] != EXT_FLASH_MID || rbuf[1] != EXT_FLASH_DEVICE_ID) {
    return VERIFY_PART_POWERED_DOWN;
  }
  return VERIFY_PART_OK;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Put the device in power save mode. No access to data; only
 *        the status register is accessible.
 */
static bool
power_down(void)
{
  uint8_t cmd;
  uint8_t i;

  /* First, wait for the device to be ready */
  if(wait_ready() == false) {
    /* Entering here will leave the device in standby instead of powerdown */
    return false;
  }

  cmd = BLS_CODE_PD;
  if(select_on_bus() == false) {
    return false;
  }

  if(spi_write_byte(flash_spi_configuration, cmd) != SPI_DEV_STATUS_OK) {
    deselect();
    return false;
  }
  deselect();

  i = 0;
  while(i < 10) {
    if(verify_part() == VERIFY_PART_POWERED_DOWN) {
      /* Device is powered down */
      return true;
    }
    i++;
  }

  /* Should not be required */
  deselect();
  return false;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief    Take device out of power save mode and prepare it for normal operation
 * \return   True if the command was written successfully
 */
static bool
power_standby(void)
{
  uint8_t cmd;
  bool success;

  cmd = BLS_CODE_RPD;
  if(select_on_bus() == false) {
    return false;
  }

  success = (spi_write(flash_spi_configuration, &cmd, sizeof(cmd)) == SPI_DEV_STATUS_OK);

  if(success) {
    success = wait_ready() == true ? true : false;
  }

  deselect();

  return success;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Enable write.
 * \return True when successful.
 */
static bool
write_enable(void)
{
  bool ret;
  const uint8_t wbuf[] = { BLS_CODE_WRITE_ENABLE };

  if(select_on_bus() == false) {
    return false;
  }

  ret = (spi_write(flash_spi_configuration, wbuf, sizeof(wbuf)) == SPI_DEV_STATUS_OK);
  deselect();

  if(ret == false) {
    return false;
  }
  return true;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_open()
{
  /* Check if platform has ext-flash */
  if(flash_spi_configuration->pin_spi_sck == 255) {
    return false;
  }

  if(spi_acquire(flash_spi_configuration) != SPI_DEV_STATUS_OK) {
    return false;
  }
  /* Default output to clear chip select */
  deselect();

  /* Put the part is standby mode */
  power_standby();

  return verify_part() == VERIFY_PART_OK ? true : false;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_close()
{
  /* Put the part in low power mode */
  if(power_down() == false) {
    return false;
  }

  if(spi_release(flash_spi_configuration) != SPI_DEV_STATUS_OK) {
    return false;
  }

  return true;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_read(uint32_t offset, uint32_t length, uint8_t *buf)
{
  uint8_t wbuf[4];
  bool ret;

  /* Wait till previous erase/program operation completes */
  if(wait_ready() == false) {
    return false;
  }

  /*
   * SPI is driven with very low frequency (1MHz < 33MHz fR spec)
   * in this implementation, hence it is not necessary to use fast read.
   */
  wbuf[0] = BLS_CODE_READ;
  wbuf[1] = (offset >> 16) & 0xff;
  wbuf[2] = (offset >> 8) & 0xff;
  wbuf[3] = offset & 0xff;

  if(select_on_bus() == false) {
    return false;
  }

  if(spi_write(flash_spi_configuration, wbuf, sizeof(wbuf)) != SPI_DEV_STATUS_OK) {
    /* failure */
    deselect();
    return false;
  }

  ret = (spi_read(flash_spi_configuration, buf, length) == SPI_DEV_STATUS_OK);

  deselect();

  return ret;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_write(uint32_t offset, uint32_t length, const uint8_t *buf)
{
  uint8_t wbuf[4];
  uint32_t ilen; /* interim length per instruction */

  while(length > 0) {
    /* Wait till previous erase/program operation completes */
    if(wait_ready() == false) {
      return false;
    }

    if(write_enable() == false) {
      return false;
    }

    ilen = EXT_FLASH_PROGRAM_PAGE_SIZE - (offset % EXT_FLASH_PROGRAM_PAGE_SIZE);
    if(length < ilen) {
      ilen = length;
    }

    wbuf[0] = BLS_CODE_PROGRAM;
    wbuf[1] = (offset >> 16) & 0xff;
    wbuf[2] = (offset >> 8) & 0xff;
    wbuf[3] = offset & 0xff;

    offset += ilen;
    length -= ilen;

    /* Upto 100ns CS hold time (which is not clear
     * whether it's application only inbetween reads)
     * is not imposed here since above instructions
     * should be enough to delay
     * as much. */
    if(select_on_bus() == false) {
      return false;
    }

    if(spi_write(flash_spi_configuration, wbuf, sizeof(wbuf)) != SPI_DEV_STATUS_OK) {
      /* failure */
      deselect();
      return false;
    }

    if(spi_write(flash_spi_configuration, buf, ilen) != SPI_DEV_STATUS_OK) {
      /* failure */
      deselect();
      return false;
    }
    buf += ilen;
    deselect();
  }

  return true;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_erase(uint32_t offset, uint32_t length)
{
  /*
   * Note that Block erase might be more efficient when the floor map
   * is well planned for OTA, but to simplify this implementation,
   * sector erase is used blindly.
   */
  uint8_t wbuf[4];
  uint32_t i, numsectors;
  uint32_t endoffset = offset + length - 1;

  offset = (offset / EXT_FLASH_ERASE_SECTOR_SIZE) * EXT_FLASH_ERASE_SECTOR_SIZE;
  numsectors = (endoffset - offset + EXT_FLASH_ERASE_SECTOR_SIZE - 1) / EXT_FLASH_ERASE_SECTOR_SIZE;

  wbuf[0] = BLS_CODE_SECTOR_ERASE;

  for(i = 0; i < numsectors; i++) {
    /* Wait till previous erase/program operation completes */
    if(wait_ready() == false) {
      return false;
    }

    if(write_enable() == false) {
      return false;
    }

    wbuf[1] = (offset >> 16) & 0xff;
    wbuf[2] = (offset >> 8) & 0xff;
    wbuf[3] = offset & 0xff;

    if(select_on_bus() == false) {
      return false;
    }

    if(spi_write(flash_spi_configuration, wbuf, sizeof(wbuf)) != SPI_DEV_STATUS_OK) {
      /* failure */
      deselect();
      return false;
    }
    deselect();

    offset += EXT_FLASH_ERASE_SECTOR_SIZE;
  }

  return true;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_test(void)
{
  flash_spi_configuration = &flash_spi_configuration_default;

  if(ext_flash_open() == false) {
    return false;
  }

  if(ext_flash_close() == false) {
    return false;
  }

  LOG_INFO("Flash test successful\n");

  return true;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_init(spi_device_t *conf)
{
  if(conf == NULL) {
    flash_spi_configuration = &flash_spi_configuration_default;
  } else {
    flash_spi_configuration = conf;
  }

  return ext_flash_test();
}
/*---------------------------------------------------------------------------*/
/** @} */

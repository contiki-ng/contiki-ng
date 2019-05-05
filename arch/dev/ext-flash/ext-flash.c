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
 * \addtogroup ext-flash
 * @{
 *
 * \file
 * Implementation of a generic external SPI flash driver
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
static const spi_device_t flash_spi_configuration_default = {
#if GPIO_HAL_PORT_PIN_NUMBERING
  .port_spi_sck = EXT_FLASH_SPI_PORT_SCK,
  .port_spi_miso = EXT_FLASH_SPI_PORT_MISO,
  .port_spi_mosi = EXT_FLASH_SPI_PORT_MOSI,
  .port_spi_cs = EXT_FLASH_SPI_PORT_CS,
#endif
  .spi_controller = EXT_FLASH_SPI_CONTROLLER,
  .pin_spi_sck = EXT_FLASH_SPI_PIN_SCK,
  .pin_spi_miso = EXT_FLASH_SPI_PIN_MISO,
  .pin_spi_mosi = EXT_FLASH_SPI_PIN_MOSI,
  .pin_spi_cs = EXT_FLASH_SPI_PIN_CS,
  .spi_bit_rate = 4000000,
  .spi_pha = 0,
  .spi_pol = 0
};
/*---------------------------------------------------------------------------*/
/**
 * Get spi configuration, return default configuration if NULL
 */
static const spi_device_t *
get_spi_conf(const spi_device_t *conf)
{
  if(conf == NULL) {
    return &flash_spi_configuration_default;
  }
  return conf;
}
/*---------------------------------------------------------------------------*/
/**
 * Clear external flash CSN line
 */
static bool
select_on_bus(const spi_device_t *flash_spi_configuration)
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
deselect(const spi_device_t *flash_spi_configuration)
{
  spi_deselect(flash_spi_configuration);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Wait till previous erase/program operation completes.
 * \return True when successful.
 */
static bool
wait_ready(const spi_device_t *flash_spi_configuration)
{
  bool ret;
  const uint8_t wbuf[1] = { BLS_CODE_READ_STATUS };

  if(select_on_bus(flash_spi_configuration) == false) {
    return false;
  }

  ret = spi_write(flash_spi_configuration, wbuf, sizeof(wbuf));

  if(ret != SPI_DEV_STATUS_OK) {
    deselect(flash_spi_configuration);
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
      deselect(flash_spi_configuration);
      return false;
    }

    if(!(buf & BLS_STATUS_BIT_BUSY)) {
      /* Now ready */
      break;
    }
  }
  deselect(flash_spi_configuration);
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
verify_part(const spi_device_t *flash_spi_configuration)
{
  const uint8_t wbuf[] = { BLS_CODE_MDID, 0xFF, 0xFF, 0x00 };
  uint8_t rbuf[2] = { 0, 0 };
  bool ret;

  if(select_on_bus(flash_spi_configuration) == false) {
    return VERIFY_PART_LOCKED;
  }

  if(spi_write(flash_spi_configuration, wbuf, sizeof(wbuf)) != SPI_DEV_STATUS_OK) {
    deselect(flash_spi_configuration);
    return VERIFY_PART_ERROR;
  }

  ret = spi_read(flash_spi_configuration, rbuf, sizeof(rbuf));
  deselect(flash_spi_configuration);
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
power_down(const spi_device_t *flash_spi_configuration)
{
  uint8_t cmd;
  uint8_t i;

  /* First, wait for the device to be ready */
  if(wait_ready(flash_spi_configuration) == false) {
    /* Entering here will leave the device in standby instead of powerdown */
    return false;
  }

  cmd = BLS_CODE_PD;
  if(select_on_bus(flash_spi_configuration) == false) {
    return false;
  }

  if(spi_write_byte(flash_spi_configuration, cmd) != SPI_DEV_STATUS_OK) {
    deselect(flash_spi_configuration);
    return false;
  }
  deselect(flash_spi_configuration);

  i = 0;
  while(i < 10) {
    if(verify_part(flash_spi_configuration) == VERIFY_PART_POWERED_DOWN) {
      /* Device is powered down */
      return true;
    }
    i++;
  }

  /* Should not be required */
  deselect(flash_spi_configuration);
  return false;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief    Take device out of power save mode and prepare it for normal operation
 * \return   True if the command was written successfully
 */
static bool
power_standby(const spi_device_t *flash_spi_configuration)
{
  uint8_t cmd;
  bool success;

  cmd = BLS_CODE_RPD;
  if(select_on_bus(flash_spi_configuration) == false) {
    return false;
  }

  success = (spi_write(flash_spi_configuration, &cmd, sizeof(cmd)) == SPI_DEV_STATUS_OK);

  if(success) {
    success = wait_ready(flash_spi_configuration) == true ? true : false;
  }

  deselect(flash_spi_configuration);

  return success;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Enable write.
 * \return True when successful.
 */
static bool
write_enable(const spi_device_t *flash_spi_configuration)
{
  bool ret;
  const uint8_t wbuf[] = { BLS_CODE_WRITE_ENABLE };

  if(select_on_bus(flash_spi_configuration) == false) {
    return false;
  }

  ret = (spi_write(flash_spi_configuration, wbuf, sizeof(wbuf)) == SPI_DEV_STATUS_OK);
  deselect(flash_spi_configuration);

  if(ret == false) {
    return false;
  }
  return true;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_open(const spi_device_t *conf)
{
  const spi_device_t *flash_spi_configuration;

  flash_spi_configuration = get_spi_conf(conf);

  /* Check if platform has ext-flash */
  if(flash_spi_configuration->pin_spi_sck == GPIO_HAL_PIN_UNKNOWN) {
    return false;
  }

  if(spi_acquire(flash_spi_configuration) != SPI_DEV_STATUS_OK) {
    return false;
  }
  /* Default output to clear chip select */
  deselect(flash_spi_configuration);

  /* Put the part is standby mode */
  power_standby(flash_spi_configuration);

  if(verify_part(flash_spi_configuration) == VERIFY_PART_OK) {
    return true;
  }

  /* Failed to verify */
  spi_release(flash_spi_configuration);
  return false;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_close(const spi_device_t *conf)
{
  bool ret;
  const spi_device_t *flash_spi_configuration;

  flash_spi_configuration = get_spi_conf(conf);

  /* Put the part in low power mode */
  ret = power_down(flash_spi_configuration);

  /* SPI is released no matter if power_down() succeeds or fails */
  if(spi_release(flash_spi_configuration) != SPI_DEV_STATUS_OK) {
    return false;
  }

  return ret;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_read(const spi_device_t *conf, uint32_t offset, uint32_t length, uint8_t *buf)
{
  uint8_t wbuf[4];
  bool ret;

  const spi_device_t *flash_spi_configuration;

  flash_spi_configuration = get_spi_conf(conf);

  /* Wait till previous erase/program operation completes */
  if(wait_ready(flash_spi_configuration) == false) {
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

  if(select_on_bus(flash_spi_configuration) == false) {
    return false;
  }

  if(spi_write(flash_spi_configuration, wbuf, sizeof(wbuf)) != SPI_DEV_STATUS_OK) {
    /* failure */
    deselect(flash_spi_configuration);
    return false;
  }

  ret = (spi_read(flash_spi_configuration, buf, length) == SPI_DEV_STATUS_OK);

  deselect(flash_spi_configuration);

  return ret;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_write(const spi_device_t *conf, uint32_t offset, uint32_t length, const uint8_t *buf)
{
  uint8_t wbuf[4];
  uint32_t ilen; /* interim length per instruction */

  const spi_device_t *flash_spi_configuration;

  flash_spi_configuration = get_spi_conf(conf);

  while(length > 0) {
    /* Wait till previous erase/program operation completes */
    if(wait_ready(flash_spi_configuration) == false) {
      return false;
    }

    if(write_enable(flash_spi_configuration) == false) {
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
    if(select_on_bus(flash_spi_configuration) == false) {
      return false;
    }

    if(spi_write(flash_spi_configuration, wbuf, sizeof(wbuf)) != SPI_DEV_STATUS_OK) {
      /* failure */
      deselect(flash_spi_configuration);
      return false;
    }

    if(spi_write(flash_spi_configuration, buf, ilen) != SPI_DEV_STATUS_OK) {
      /* failure */
      deselect(flash_spi_configuration);
      return false;
    }
    buf += ilen;
    deselect(flash_spi_configuration);
  }

  return true;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_erase(const spi_device_t *conf, uint32_t offset, uint32_t length)
{
  /*
   * Note that Block erase might be more efficient when the floor map
   * is well planned for OTA, but to simplify this implementation,
   * sector erase is used blindly.
   */
  uint8_t wbuf[4];
  uint32_t i, numsectors;
  uint32_t endoffset = offset + length - 1;

  const spi_device_t *flash_spi_configuration;

  flash_spi_configuration = get_spi_conf(conf);

  offset = (offset / EXT_FLASH_ERASE_SECTOR_SIZE) * EXT_FLASH_ERASE_SECTOR_SIZE;
  numsectors = (endoffset - offset + EXT_FLASH_ERASE_SECTOR_SIZE - 1) / EXT_FLASH_ERASE_SECTOR_SIZE;

  wbuf[0] = BLS_CODE_SECTOR_ERASE;

  for(i = 0; i < numsectors; i++) {
    /* Wait till previous erase/program operation completes */
    if(wait_ready(flash_spi_configuration) == false) {
      return false;
    }

    if(write_enable(flash_spi_configuration) == false) {
      return false;
    }

    wbuf[1] = (offset >> 16) & 0xff;
    wbuf[2] = (offset >> 8) & 0xff;
    wbuf[3] = offset & 0xff;

    if(select_on_bus(flash_spi_configuration) == false) {
      return false;
    }

    if(spi_write(flash_spi_configuration, wbuf, sizeof(wbuf)) != SPI_DEV_STATUS_OK) {
      /* failure */
      deselect(flash_spi_configuration);
      return false;
    }
    deselect(flash_spi_configuration);

    offset += EXT_FLASH_ERASE_SECTOR_SIZE;
  }

  return true;
}
/*---------------------------------------------------------------------------*/
bool
ext_flash_init(const spi_device_t *conf)
{
  if(ext_flash_open(conf) == false) {
    return false;
  }

  if(ext_flash_close(conf) == false) {
    return false;
  }

  LOG_INFO("Flash init successful\n");

  return true;
}
/*---------------------------------------------------------------------------*/
/** @} */

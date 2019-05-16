/*
 * Copyright (c) 2016-2017, Yanzi Networks.
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup dev
 * @{
 *
 * \defgroup spi-hal SPI Hardware Abstraction Layer
 *
 * The SPI HAL provides a set of common functions that can be used in a
 * platform-independent fashion.
 *
 *
 * @{
 *
 * \file
 *     Header file for the SPI HAL
 */
/*---------------------------------------------------------------------------*/
#ifndef SPI_H_
#define SPI_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "gpio-hal.h"

#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
/* Include Arch-Specific conf */
#ifdef SPI_HAL_CONF_ARCH_HDR_PATH
#include SPI_HAL_CONF_ARCH_HDR_PATH
#endif /* SPI_HAL_CONF_ARCH_HDR_PATH */
/*---------------------------------------------------------------------------*/
#ifdef SPI_CONF_CONTROLLER_COUNT
/**
 * \brief Number of SPI module instances on a chip
 */
#define SPI_CONTROLLER_COUNT SPI_CONF_CONTROLLER_COUNT
#else
#define SPI_CONTROLLER_COUNT 0
#endif
/*---------------------------------------------------------------------------*/
/* Convenience macros to enumerate SPI module instances on a chip */
#define SPI_CONTROLLER_SPI0 0
#define SPI_CONTROLLER_SPI1 1
/*---------------------------------------------------------------------------*/
/**
 * \brief SPI return codes
 *
 * @{
 */
typedef enum {
  SPI_DEV_STATUS_OK,              /* Everything OK */
  SPI_DEV_STATUS_EINVAL,          /* Erroneous input value */
  SPI_DEV_STATUS_TRANSFER_ERR,    /* Error during SPI transfer */
  SPI_DEV_STATUS_BUS_LOCKED,      /* SPI bus is already locked */
  SPI_DEV_STATUS_BUS_NOT_OWNED,   /* SPI bus is locked by someone else */
  SPI_DEV_STATUS_CLOSED           /* SPI bus has not opened properly */
} spi_status_t;
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \brief SPI Device Configuration
 *
 * This is a structure to an architecture-independent SPI configuration.
 *
 * \note Do not access the port_spi_foo members directly. Access them by using
 * the SPI_DEVICE_PORT() macro instead
 * @{
 */

typedef struct spi_device {
#if GPIO_HAL_PORT_PIN_NUMBERING
  gpio_hal_port_t port_spi_sck;       /* SPI SCK port */
  gpio_hal_port_t port_spi_miso;      /* SPI MISO port */
  gpio_hal_port_t port_spi_mosi;      /* SPI MOSI port */
  gpio_hal_port_t port_spi_cs;        /* SPI Chip Select port */
#endif
  gpio_hal_pin_t pin_spi_sck;       /* SPI SCK pin */
  gpio_hal_pin_t pin_spi_miso;      /* SPI MISO  pin */
  gpio_hal_pin_t pin_spi_mosi;      /* SPI MOSI pin */
  gpio_hal_pin_t pin_spi_cs;        /* SPI Chip Select pin */
  uint32_t spi_bit_rate;            /* SPI bit rate */
  uint8_t spi_pha;                  /* SPI mode phase */
  uint8_t spi_pol;                  /* SPI mode polarity */
  uint8_t spi_controller;           /* ID of SPI controller to use */
} spi_device_t;
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \brief Retrieve the SPI device's port number if applicable
 * \param member Retrieve struct member port_spi_member (e.g. port_spi_miso)
 * \param device A pointer the a variable of type spi_device_t
 *
 * The same macro is used for all four port_spi_foo members of the struct. So
 * to retrieve port_spi_cs, use SPI_DEVICE_PORT(cs, device). Replace cs with
 * mosi to retrieve port_spi_mosi.
 *
 */
#if GPIO_HAL_PORT_PIN_NUMBERING
#define SPI_DEVICE_PORT(member, device) (device)->port_spi_##member
#else
#define SPI_DEVICE_PORT(member, device) GPIO_HAL_NULL_PORT
#endif
/*---------------------------------------------------------------------------*/
/* These are architecture-independent functions to be used by SPI devices.   */
/*---------------------------------------------------------------------------*/
/**
 * \brief Locks and then opens an SPI controller
 * \param dev An SPI device configuration which defines the controller
 * to be locked and the opening configuration.
 * \return SPI return code
 */
spi_status_t spi_acquire(const spi_device_t *dev);

/**
 * \brief Closes and then unlocks an SPI controller
 * \param dev An SPI device configuration which defines the controller
 * to be closed and unlocked.
 * \return SPI return code
 *
 * Releasing an SPI controller should put it in low-power mode.
 * This should work only if the device has already locked the SPI
 * controller.
 */
spi_status_t spi_release(const spi_device_t *dev);

/**
 * \brief Selects the SPI peripheral
 * \param dev An SPI device configuration which defines the CS pin.
 * \return SPI return code
 *
 * Clears the CS pin. This should work only if the device has
 * already locked the SPI controller.
 */
spi_status_t spi_select(const spi_device_t *dev);

/**
 * \brief Deselects the SPI peripheral
 * \param dev An SPI device configuration which defines the CS pin.
 * \return SPI return code
 *
 * Sets the CS pin. Lock is not required.
 */
spi_status_t spi_deselect(const spi_device_t *dev);

/**
 * \brief Checks if a device has locked an SPI controller
 * \param dev An SPI device configuration which defines the controller.
 * \return true if the device has the lock, false otherwise.
 */
bool spi_has_bus(const spi_device_t *dev);

/**
 * \brief Writes a single byte to an SPI device
 * \param dev An SPI device configuration.
 * \param data A byte of data
 * \return SPI return code
 *
 * It should work only if the device has already locked the SPI controller.
 */
spi_status_t spi_write_byte(const spi_device_t *dev, uint8_t data);

/**
 * \brief Reads a single byte from an SPI device
 * \param dev An SPI device configuration.
 * \param data A pointer to a byte of data
 * \return SPI return code
 *
 * It should work only if the device has already locked the SPI controller.
 */
spi_status_t spi_read_byte(const spi_device_t *dev, uint8_t *data);

/**
 * \brief Writes a buffer to an SPI device
 * \param dev An SPI device configuration.
 * \param data A pointer to the data
 * \param size Size of the data to write
 * \return SPI return code
 *
 * It should work only if the device has already locked the SPI controller.
 */
spi_status_t spi_write(const spi_device_t *dev,
                       const uint8_t *data, int size);

/**
 * \brief Reads a buffer from an SPI device
 * \param dev An SPI device configuration.
 * \param data A pointer to the data
 * \param size Size of the data to read
 * \return SPI return code
 *
 * It should work only if the device has already locked the SPI controller.
 */
spi_status_t spi_read(const spi_device_t *dev, uint8_t *data, int size);

/**
 * \brief Reads and ignores data from an SPI device
 * \param dev An SPI device configuration.
 * \param size Size of the data to read and ignore
 * \return SPI return code
 *
 * Reads size bytes from the SPI and throws them away.
 * It should work only if the device has already locked the SPI controller.
 */
spi_status_t spi_read_skip(const spi_device_t *dev, int size);

/**
 * \brief Performs a generic SPI transfer
 * \param dev An SPI device configuration.
 * \param data A pointer to the data to be written. Set it to NULL to
 * skip writing.
 * \param wsize Size of data to write.
 * \param buf A pointer to buffer to copy the data read. Set to NULL
 * to skip reading.
 * \param rsize Size of data to read.
 * \param ignore Size of data to read and ignore.
 * \return SPI return code
 *
 * It should work only if the device has already locked the SPI controller.
 * A total of rlen+ignore_len bytes will be read. The first rlen bytes will
 * be copied to buf. The remaining ignore_len bytes won't be copied to the
 * buffer. The maximum of wlen and rlen+ignore_len of bytes will be transfered.
 */
spi_status_t spi_transfer(const spi_device_t *dev,
                          const uint8_t *data, int wsize,
                          uint8_t *buf, int rsize, int ignore);

/**
 * \brief Reads and Writes one byte from/to an SPI device
 * \param dev An SPI device configuration.
 * \param strobe Byte to write
 * \param status Pointer to byte to read
 * \return SPI return code
 *
 * It should work only if the device has already locked the SPI controller.
 */
spi_status_t spi_strobe(const spi_device_t *dev, uint8_t strobe,
                        uint8_t *status);

/**
 * \brief Reads a buffer of bytes from a register of an SPI device
 * \param dev An SPI device configuration.
 * \param reg Register
 * \param data A pointer to the data
 * \param size Size of the data to read
 * \return SPI return code
 *
 * It should work only if the device has already locked the SPI controller.
 */
spi_status_t spi_read_register(const spi_device_t *dev, uint8_t reg,
                               uint8_t *data, int size);

/*---------------------------------------------------------------------------*/
/* These are architecture-specific functions to be implemented by each CPU.   */
/*---------------------------------------------------------------------------*/

/**
 * \brief Checks if a device has locked an SPI controller
 * \param dev An SPI device configuration which defines the controller
 * to be checked if it is locked and the respective device.
 * \return 1 if the device has the lock, 0 otherwise.
 *
 */
bool spi_arch_has_lock(const spi_device_t *dev);

/**
 * \brief Checks if an SPI controller is locked by any device
 * \param dev An SPI device configuration which defines the controller
 * to be checked.
 * \return 1 if the controller is locked, 0 otherwise.
 *
 */
bool spi_arch_is_bus_locked(const spi_device_t *dev);

/**
 * \brief Locks and opens an SPI controller to the configuration specified.
 * \param dev An SPI device configuration.
 * \return SPI return code
 *
 * This should work only if the device has already locked the SPI
 * controller.
 *
 */
spi_status_t spi_arch_lock_and_open(const spi_device_t *dev);

/**
 * \brief Closes and unlocks an SPI controller
 * \param dev An SPI device configuration that specifies the controller.
 * \return SPI return code
 *
 * This should turn off the SPI controller to put it in low power mode
 * and unlock it.
 * It should work only if the device has already locked the SPI
 * controller.
 *
 */
spi_status_t spi_arch_close_and_unlock(const spi_device_t *dev);

/**
 * \brief Performs an SPI transfer
 * \param dev An SPI device configuration that specifies the controller.
 * \param data A pointer to the data to be written. Set it to NULL to
 * skip writing.
 * \param wlen Length of data to write.
 * \param buf A pointer to buffer to copy the data read. Set to NULL
 * to skip reading.
 * \param rlen Length of data to read.
 * \param ignore_len Length of data to read and ignore.
 * \return SPI return code
 *
 * It should work only if the device has already locked the SPI controller.
 * A total of rlen+ignore_len bytes will be read. The first rlen bytes will
 * be copied to buf. The remaining ignore_len bytes won't be copied to the
 * buffer. The maximum of wlen and rlen+ignore_len of bytes will be transfered.
 */
spi_status_t spi_arch_transfer(const spi_device_t *dev,
                               const uint8_t *data, int wlen,
                               uint8_t *buf, int rlen,
                               int ignore_len);

#endif /* SPI_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

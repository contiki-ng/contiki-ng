/*
 * Copyright (c) 2016, Michael Spoerk
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
 * \addtogroup cc13xx-cc26xx-rf-ble-addr
 * @{
 *
 * \file
 *        Implementation of the CC13xx/CC26xx IEEE addresses driver.
 * \author
 *        Michael Spoerk <mi.spoerk@gmail.com>
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/ble-hal.h"
#include "net/linkaddr.h"

#include "rf/ble-addr.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_memmap.h)
#include DeviceFamily_constructPath(inc/hw_fcfg1.h)
#include DeviceFamily_constructPath(inc/hw_ccfg.h)
/*---------------------------------------------------------------------------*/
#include <string.h>
/*---------------------------------------------------------------------------*/
#define BLE_MAC_PRIMARY_ADDRESS    (FCFG1_BASE + FCFG1_O_MAC_BLE_0)
#define BLE_MAC_SECONDARY_ADDRESS  (CCFG_BASE + CCFG_O_IEEE_BLE_0)
/*---------------------------------------------------------------------------*/
uint8_t *
ble_addr_ptr(void)
{
  volatile const uint8_t *const primary = (uint8_t *)BLE_MAC_PRIMARY_ADDRESS;
  volatile const uint8_t *const secondary = (uint8_t *)BLE_MAC_SECONDARY_ADDRESS;

  /*
   * Reading from primary location...
   * ...unless we can find a byte != 0xFF in secondary
   *
   * Intentionally checking all bytes here instead of len, because we
   * are checking validity of the entire address irrespective of the
   * actual number of bytes the caller wants to copy over.
   */
  size_t i;
  for(i = 0; i < BLE_ADDR_SIZE; i++) {
    if(secondary[i] != 0xFF) {
      /* A byte in secondary is not 0xFF. Use secondary address. */
      return (uint8_t *)secondary;
    }
  }

  /* All bytes in secondary is 0xFF. Use primary address. */
  return (uint8_t *)primary;
}
/*---------------------------------------------------------------------------*/
int
ble_addr_be_cpy(uint8_t *dst)
{
  if(!dst) {
    return -1;
  }

  volatile const uint8_t *const ble_addr = ble_addr_ptr();

  /*
   * We have chosen what address to read the BLE address from. Do so,
   * inverting byte order
   */
  size_t i;
  for(i = 0; i < BLE_ADDR_SIZE; i++) {
    dst[i] = ble_addr[BLE_ADDR_SIZE - 1 - i];
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
int
ble_addr_le_cpy(uint8_t *dst)
{
  if(!dst) {
    return -1;
  }

  volatile const uint8_t *const ble_addr = ble_addr_ptr();

  size_t i;
  for(i = 0; i < BLE_ADDR_SIZE; i++) {
    dst[i] = ble_addr[i];
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
int
ble_addr_to_eui64(uint8_t *dst, uint8_t *src)
{
  if(!dst || !src) {
    return -1;
  }

  memcpy(dst, src, 3);
  dst[3] = 0xFF;
  dst[4] = 0xFE;
  memcpy(&dst[5], &src[3], 3);

  return 0;
}
/*---------------------------------------------------------------------------*/
int
ble_addr_to_eui64_cpy(uint8_t *dst)
{
  if(!dst) {
    return -1;
  }

  int res;
  uint8_t ble_addr[BLE_ADDR_SIZE];

  res = ble_addr_le_cpy(ble_addr);
  if(res) {
    return -1;
  }

  return ble_addr_to_eui64(dst, ble_addr);
}
/*---------------------------------------------------------------------------*/
/** @} */

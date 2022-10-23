/*
 * Copyright (c) 2016, Benoît Thébaudeau <benoit@wsystem.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \addtogroup remote-fat
 * @{
 *
 * \file
 * Implementation of the default port of FatFs on RE-Mote.
 */
#include "diskio.h"
#include "dev/storage/disk/mmc/mmc.h"
#include "rtcc.h"

/*----------------------------------------------------------------------------*/
DSTATUS __attribute__((__weak__))
disk_status(BYTE pdrv)
{
  return ~mmc_driver.status(pdrv);
}
/*----------------------------------------------------------------------------*/
DSTATUS __attribute__((__weak__))
disk_initialize(BYTE pdrv)
{
  return ~mmc_driver.initialize(pdrv);
}
/*----------------------------------------------------------------------------*/
DRESULT __attribute__((__weak__))
disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
  switch(mmc_driver.read(pdrv, buff, sector, count)) {
  default:
  case DISK_RESULT_NO_INIT:
  case DISK_RESULT_IO_ERROR: return RES_ERROR;
  case DISK_RESULT_OK: return RES_OK;
  case DISK_RESULT_WR_PROTECTED: return RES_WRPRT;
  case DISK_RESULT_INVALID_ARG: return RES_PARERR;
  }
}
/*----------------------------------------------------------------------------*/
DRESULT __attribute__((__weak__))
disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
  switch(mmc_driver.write(pdrv, buff, sector, count)) {
  default:
  case DISK_RESULT_NO_INIT:
  case DISK_RESULT_IO_ERROR: return RES_ERROR;
  case DISK_RESULT_OK: return RES_OK;
  case DISK_RESULT_WR_PROTECTED: return RES_WRPRT;
  case DISK_RESULT_INVALID_ARG: return RES_PARERR;
  }
}
/*----------------------------------------------------------------------------*/
DRESULT __attribute__((__weak__))
disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
  switch(mmc_driver.ioctl(pdrv, cmd, buff)) {
  default:
  case DISK_RESULT_NO_INIT:
  case DISK_RESULT_IO_ERROR: return RES_ERROR;
  case DISK_RESULT_OK: return RES_OK;
  case DISK_RESULT_WR_PROTECTED: return RES_WRPRT;
  case DISK_RESULT_INVALID_ARG: return RES_PARERR;
  }
}
/*----------------------------------------------------------------------------*/
DWORD __attribute__((__weak__))
get_fattime(void)
{
  simple_td_map td;

  return rtcc_get_time_date(&td) == AB08_SUCCESS ?
    (2000 + td.years - 1980) << 25 | td.months << 21 | td.day << 16 |
    td.hours << 11 | td.minutes << 5 | td.seconds : 0;
}
/*----------------------------------------------------------------------------*/

/** @} */

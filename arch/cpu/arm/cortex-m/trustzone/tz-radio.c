/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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

/*
 * \file
 *   TrustZone secure-world radio NSC entry points.
 *
 *   These functions are marked as Non-Secure Callable (NSC) so
 *   the normal world can invoke radio operations through the
 *   secure world. All normal-world pointers are validated via
 *   CMSE before use.
 *
 * \author
 *   Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "contiki.h"
#include "dev/radio.h"
#include "tz-radio.h"

#include <arm_cmse.h>
#include <string.h>

#include "nrf.h"
#include "nrf_ficr.h"

/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "TZRadio"
#define LOG_LEVEL LOG_LEVEL_INFO
/*---------------------------------------------------------------------------*/
/* Maximum payload and object sizes for secure-side copy buffers. */
#define TZ_RADIO_MAX_PAYLOAD  128
#define TZ_RADIO_MAX_OBJECT   140
#define TZ_RADIO_BUF_SIZE \
  (TZ_RADIO_MAX_OBJECT > TZ_RADIO_MAX_PAYLOAD \
   ? TZ_RADIO_MAX_OBJECT : TZ_RADIO_MAX_PAYLOAD)
/*---------------------------------------------------------------------------*/
extern const struct radio_driver ipc_radio_driver;
/*---------------------------------------------------------------------------*/
static tz_radio_ns_rx_callback_t ns_rx_callback;
static bool rx_callback_registered;
/*---------------------------------------------------------------------------*/
/* RX attributes stored by tz_radio_notify_rx for normal-world retrieval. */
static int8_t last_rx_rssi;
static uint8_t last_rx_lqi;
/*---------------------------------------------------------------------------*/
/*
 * A single static bounce buffer shared by all NSC entry points that copy
 * data across the secure/non-secure boundary. These entry points are only
 * invoked by the single-threaded, cooperative normal-world scheduler, so at
 * most one secure radio call runs at a time and the buffer is never used by
 * two calls concurrently. It is sized for the largest transfer.
 */
static uint8_t secure_buf[TZ_RADIO_BUF_SIZE];
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL int
tz_radio_init(void)
{
  return ipc_radio_driver.init();
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL int
tz_radio_prepare(const void *payload, unsigned short payload_len)
{
  if(payload_len > TZ_RADIO_MAX_PAYLOAD) {
    return 1;
  }

  if(cmse_check_address_range((void *)payload, payload_len,
                              CMSE_NONSECURE) == NULL) {
    LOG_ERR("prepare: invalid NS pointer\n");
    return 1;
  }

  memcpy(secure_buf, payload, payload_len);

  return ipc_radio_driver.prepare(secure_buf, payload_len);
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL int
tz_radio_transmit(unsigned short transmit_len)
{
  return ipc_radio_driver.transmit(transmit_len);
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL int
tz_radio_send(const void *payload, unsigned short payload_len)
{
  if(payload_len > TZ_RADIO_MAX_PAYLOAD) {
    return RADIO_TX_ERR;
  }

  if(cmse_check_address_range((void *)payload, payload_len,
                              CMSE_NONSECURE) == NULL) {
    LOG_ERR("send: invalid NS pointer\n");
    return RADIO_TX_ERR;
  }

  memcpy(secure_buf, payload, payload_len);

  return ipc_radio_driver.send(secure_buf, payload_len);
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL int
tz_radio_read(void *buf, unsigned short buf_len)
{
  int len;

  if(cmse_check_address_range(buf, buf_len, CMSE_NONSECURE) == NULL) {
    LOG_ERR("read: invalid NS pointer\n");
    return 0;
  }

  len = ipc_radio_driver.read(secure_buf,
                              buf_len < TZ_RADIO_MAX_PAYLOAD
                              ? buf_len : TZ_RADIO_MAX_PAYLOAD);
  if(len > 0) {
    memcpy(buf, secure_buf, len);
  }

  return len;
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL int
tz_radio_channel_clear(void)
{
  return ipc_radio_driver.channel_clear();
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL int
tz_radio_receiving_packet(void)
{
  return ipc_radio_driver.receiving_packet();
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL int
tz_radio_pending_packet(void)
{
  return ipc_radio_driver.pending_packet();
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL int
tz_radio_on(void)
{
  return ipc_radio_driver.on();
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL int
tz_radio_off(void)
{
  return ipc_radio_driver.off();
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL radio_result_t
tz_radio_get_value(radio_param_t param, radio_value_t *value)
{
  radio_value_t secure_value;
  radio_result_t result;

  if(cmse_check_address_range(value, sizeof(*value),
                              CMSE_NONSECURE) == NULL) {
    LOG_ERR("get_value: invalid NS pointer\n");
    return RADIO_RESULT_ERROR;
  }

  result = ipc_radio_driver.get_value(param, &secure_value);
  if(result == RADIO_RESULT_OK) {
    *value = secure_value;
  }

  return result;
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL radio_result_t
tz_radio_set_value(radio_param_t param, radio_value_t value)
{
  return ipc_radio_driver.set_value(param, value);
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL radio_result_t
tz_radio_get_object(radio_param_t param, void *dest, size_t size)
{
  radio_result_t result;

  if(size > TZ_RADIO_MAX_OBJECT) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  if(cmse_check_address_range(dest, size, CMSE_NONSECURE) == NULL) {
    LOG_ERR("get_object: invalid NS pointer\n");
    return RADIO_RESULT_ERROR;
  }

  result = ipc_radio_driver.get_object(param, secure_buf, size);
  if(result == RADIO_RESULT_OK) {
    memcpy(dest, secure_buf, size);
  }

  return result;
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL radio_result_t
tz_radio_set_object(radio_param_t param, const void *src, size_t size)
{
  if(size > TZ_RADIO_MAX_OBJECT) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  if(cmse_check_address_range((void *)src, size, CMSE_NONSECURE) == NULL) {
    LOG_ERR("set_object: invalid NS pointer\n");
    return RADIO_RESULT_ERROR;
  }

  memcpy(secure_buf, src, size);

  return ipc_radio_driver.set_object(param, secure_buf, size);
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL bool
tz_radio_get_device_id(uint32_t *id0, uint32_t *id1)
{
  if(cmse_check_address_range(id0, sizeof(*id0),
                              CMSE_NONSECURE) == NULL) {
    return false;
  }
  if(cmse_check_address_range(id1, sizeof(*id1),
                              CMSE_NONSECURE) == NULL) {
    return false;
  }

#if defined(NRF_FICR_S)
  *id0 = nrf_ficr_deviceid_get(NRF_FICR_S, 0);
  *id1 = nrf_ficr_deviceid_get(NRF_FICR_S, 1);
#elif defined(NRF_FICR)
  *id0 = nrf_ficr_deviceid_get(NRF_FICR, 0);
  *id1 = nrf_ficr_deviceid_get(NRF_FICR, 1);
#else
  *id0 = 0;
  *id1 = 0;
#endif

  return true;
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL bool
tz_radio_get_rx_attributes(int8_t *rssi, uint8_t *lqi)
{
  if(cmse_check_address_range(rssi, sizeof(*rssi),
                              CMSE_NONSECURE) == NULL) {
    return false;
  }
  if(cmse_check_address_range(lqi, sizeof(*lqi),
                              CMSE_NONSECURE) == NULL) {
    return false;
  }

  *rssi = last_rx_rssi;
  *lqi = last_rx_lqi;
  return true;
}
/*---------------------------------------------------------------------------*/
CC_TRUSTZONE_SECURE_CALL bool
tz_radio_register_rx_callback(tz_radio_ns_rx_callback_t callback)
{
  if(rx_callback_registered) {
    return false;
  }

  if(cmse_check_address_range((void *)callback, sizeof(callback),
                              CMSE_NONSECURE) == NULL) {
    LOG_ERR("register_rx_callback: invalid NS function pointer\n");
    return false;
  }

  /* Tag the function pointer as non-secure so the eventual call goes
   * through BLXNS with the correct state transition. cmse_check_address_range
   * only validates the address; cmse_nsfptr_create sets the NS attribute. */
  ns_rx_callback =
    (tz_radio_ns_rx_callback_t)cmse_nsfptr_create((void *)callback);
  rx_callback_registered = true;

  LOG_INFO("RX callback registered\n");
  return true;
}
/*---------------------------------------------------------------------------*/
void
tz_radio_notify_rx(int8_t rssi, uint8_t lqi)
{
  last_rx_rssi = rssi;
  last_rx_lqi = lqi;

  if(rx_callback_registered && cmse_is_nsfptr((void *)ns_rx_callback)) {
    ns_rx_callback();
  }
}
/*---------------------------------------------------------------------------*/

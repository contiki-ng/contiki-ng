/*
 * Copyright (C) 2021 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-dev Device drivers
 * @{
 *
 * \addtogroup nrf-usb USB driver
 * @{
 *
 * \file
 *         USB implementation for the nRF.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#if NRF_HAS_USB
/*---------------------------------------------------------------------------*/
#include "usb.h"
#include "usb_descriptors.h"

#include "nrfx.h"
#include "nrfx_power.h"
#include "nrf_ficr.h"
#include "nrf_power.h"
#include "nrf_gpio.h"

#include "tusb_config.h"
#include "tusb.h"

#include "sys/process.h"

PROCESS_NAME(usb_arch_process);
/*---------------------------------------------------------------------------*/
extern void tusb_hal_nrf_power_event(uint32_t event);
/*---------------------------------------------------------------------------*/
#define SERIAL_NUMBER_STRING_SIZE 12
/*---------------------------------------------------------------------------*/
static char serial[SERIAL_NUMBER_STRING_SIZE + 1];
/*---------------------------------------------------------------------------*/
void
USBD_IRQHandler(void)
{
  usb_interrupt_handler();
}
/*---------------------------------------------------------------------------*/
static void
power_event_handler(nrfx_power_usb_evt_t event)
{
  tusb_hal_nrf_power_event((uint32_t)event);
}
/*---------------------------------------------------------------------------*/
void
usb_arch_init(void)
{
  /* On nRF52 series, FICR exposes DEVICEADDR -- the 48-bit BLE-style
   * address. Use it so the USB iSerialNumber matches the value Nordic's
   * open-bootloader and the old arch/platform/nrf52840 USB stack
   * (app_usbd_serial_num_generate) report; the OR with 0xC000 mirrors
   * how the SDK turns it into a static-random BLE address.
   *
   * On nRF5340/nRF54 series, FICR has no DEVICEADDR -- only an INFO
   * struct with a DEVICEID. Fall back to that on those CPUs. The
   * serial number won't match the nRF52 bootloader-vs-app trick (there
   * is no comparable bootloader on those parts anyway), but it stays
   * unique per device, which is all the descriptor needs. */
#if defined(NRF52_SERIES) || defined(NRF52840_XXAA) \
  || defined(NRF52833_XXAA) || defined(NRF52832_XXAA) \
  || defined(NRF52820_XXAA) || defined(NRF52811_XXAA) \
  || defined(NRF52810_XXAA) || defined(NRF52805_XXAA)
  const uint16_t serial_num_high_bytes =
    (uint16_t)NRF_FICR->DEVICEADDR[1] | 0xC000;
  const uint32_t serial_num_low_bytes = NRF_FICR->DEVICEADDR[0];
#else
  const uint16_t serial_num_high_bytes =
    (uint16_t)NRF_FICR->INFO.DEVICEID[1] | 0xC000;
  const uint32_t serial_num_low_bytes = NRF_FICR->INFO.DEVICEID[0];
#endif
  const nrfx_power_config_t power_config = { 0 };
  const nrfx_power_usbevt_config_t power_usbevt_config = {
    .handler = power_event_handler
  };

  nrfx_power_init(&power_config);

  nrfx_power_usbevt_init(&power_usbevt_config);

  nrfx_power_usbevt_enable();

  // Set up descriptor
  snprintf(serial,
                  SERIAL_NUMBER_STRING_SIZE + 1,
                  "%04"PRIX16"%08"PRIX32,
                  serial_num_high_bytes,
                  serial_num_low_bytes);

  usb_descriptor_set_serial(serial);

  nrfx_power_usb_state_t usb_reg = nrfx_power_usbstatus_get();
  if(usb_reg == NRFX_POWER_USB_STATE_CONNECTED) {
    tusb_hal_nrf_power_event(NRFX_POWER_USB_EVT_DETECTED);
  } else if(usb_reg == NRFX_POWER_USB_STATE_READY) {
    tusb_hal_nrf_power_event(NRFX_POWER_USB_EVT_READY);
  }
}
/*---------------------------------------------------------------------------*/
#if CFG_TUD_DFU_RUNTIME
/*
 * Standard USB DFU runtime detach handler. Set the Nordic open-bootloader
 * retention pattern in GPREGRET so the bootloader stays in DFU mode after
 * the system reset, and reboot. Mirrors the behavior of the old nrf52840
 * platform's app_usbd_nrf_dfu_trigger-based handler in
 * arch/cpu/nrf52840/usb/usb-dfu-trigger.c.
 */
#define BOOTLOADER_DFU_GPREGRET_MAGIC   0xB0u
#define BOOTLOADER_DFU_START_BIT        0x01u
#define BOOTLOADER_DFU_START            (BOOTLOADER_DFU_GPREGRET_MAGIC | BOOTLOADER_DFU_START_BIT)

#ifndef BOARD_DFU_SELF_RESET_PIN
/* nRF52840 Dongle (PCA10059) has P0.19 solder-bridged to the chip's
 * RESET pin. Driving it low triggers a pin reset, which the dongle's
 * open-bootloader treats as a request to enter DFU mode. Boards without
 * this hardware can override BOARD_DFU_SELF_RESET_PIN to -1 to skip the
 * pin-reset path. */
#define BOARD_DFU_SELF_RESET_PIN  NRF_GPIO_PIN_MAP(0, 19)
#endif

void
tud_dfu_runtime_reboot_to_dfu_cb(void)
{
  /* Drive the self-reset GPIO low. On PCA10059 the GP pin is solder-
   * bridged to the chip's nRESET line, so this causes a hardware pin
   * reset within microseconds and the bootloader (which sees
   * RESETREAS.RESETPIN set) enters DFU mode. Mirrors what the OLD
   * arch/cpu/nrf52840/usb/usb-dfu-trigger.c did and what RIOT-OS's
   * boards/nrf52840dongle/reset.c does. The control transfer's USB
   * ACK won't reach the host (the device disappears mid-transfer),
   * but that's the expected behavior for bitWillDetach. */
#if BOARD_DFU_SELF_RESET_PIN >= 0
  nrf_gpio_cfg_output(BOARD_DFU_SELF_RESET_PIN);
  nrf_gpio_pin_clear(BOARD_DFU_SELF_RESET_PIN);
#endif

  /* Belt-and-braces for boards without the GP-pin-to-RESET wiring:
   * also set Nordic's GPREGRET DFU magic and trigger a soft reset.
   * Won't run on PCA10059 because the pin-reset above is faster. */
  nrf_power_gpregret_set(NRF_POWER, 0, BOOTLOADER_DFU_START);
  NVIC_SystemReset();
}
#endif /* CFG_TUD_DFU_RUNTIME */
/*---------------------------------------------------------------------------*/
/*
 * Nordic-vendor-specific DFU trigger interface, mirroring what the OLD
 * arch/cpu/nrf52840/usb/usb-dfu-trigger.c exposed via Nordic SDK's
 * app_usbd_nrf_dfu_trigger. Lets `nrfutil dfu usb-serial` and other
 * Nordic-aware host tools reboot the dongle into Open DFU Bootloader
 * without touching the physical RESET button.
 *
 * Wire format (from app_usbd_nrf_dfu_trigger_types.h):
 *   Interface  class/subclass/protocol = 0xFF / 0x01 / 0x01
 *   Functional descriptor type         = 0x21 (CS_FUNCTIONAL), 9 bytes
 *   Control requests on this interface:
 *     bRequest 0x00  DETACH        (host->dev, no data, triggers reboot)
 *     bRequest 0x07  NORDIC_INFO   (dev->host, 24 bytes of dfu_nordic_info)
 *     bRequest 0x08  SEM_VER       (dev->host, ASCII version string)
 */
#include "device/usbd_pvt.h"

#define NORDIC_DFU_TRIGGER_CLASS        0xFFu
#define NORDIC_DFU_TRIGGER_SUBCLASS     0x01u
#define NORDIC_DFU_TRIGGER_PROTOCOL     0x01u
#define NORDIC_DFU_TRIGGER_CS_FUNCTIONAL 0x21u
#define NORDIC_DFU_TRIGGER_REQ_DETACH      0x00u
#define NORDIC_DFU_TRIGGER_REQ_NORDIC_INFO 0x07u
#define NORDIC_DFU_TRIGGER_REQ_SEM_VER     0x08u

struct nordic_dfu_info {
  uint32_t wAddress;
  uint32_t wFirmwareSize;
  uint16_t wVersionMajor;
  uint16_t wVersionMinor;
  uint32_t wFirmwareID;
  uint32_t wFlashSize;
  uint32_t wFlashPageSize;
} __attribute__((packed));

static const struct nordic_dfu_info nordic_info = {
  .wAddress = 0x1000u,            /* App start (after Nordic MBR) */
  .wFirmwareSize = 0u,            /* Unknown at runtime; left zero */
  .wVersionMajor = 1u,
  .wVersionMinor = 0u,
  .wFirmwareID = 0u,
  .wFlashSize = 1024u * 1024u,    /* nRF52840 has 1 MiB flash */
  .wFlashPageSize = 4096u,
};

static const char nordic_sem_ver[] = "Contiki-NG DFU";

static void
nordic_dfu_trigger_init(void)
{
}
/*---------------------------------------------------------------------------*/
static void
nordic_dfu_trigger_reset(uint8_t rhport)
{
  (void)rhport;
}
/*---------------------------------------------------------------------------*/
static uint16_t
nordic_dfu_trigger_open(uint8_t rhport,
                       tusb_desc_interface_t const *itf_desc,
                       uint16_t max_len)
{
  (void)rhport;
  (void)max_len;

  if(itf_desc->bInterfaceClass != NORDIC_DFU_TRIGGER_CLASS
     || itf_desc->bInterfaceSubClass != NORDIC_DFU_TRIGGER_SUBCLASS
     || itf_desc->bInterfaceProtocol != NORDIC_DFU_TRIGGER_PROTOCOL) {
    return 0;
  }

  uint16_t drv_len = sizeof(tusb_desc_interface_t);
  uint8_t const *p = tu_desc_next(itf_desc);

  /* Optional Nordic functional descriptor (type 0x21, 9 bytes). */
  if(tu_desc_type(p) == NORDIC_DFU_TRIGGER_CS_FUNCTIONAL) {
    drv_len += tu_desc_len(p);
  }

  return drv_len;
}
/*---------------------------------------------------------------------------*/
static bool
nordic_dfu_trigger_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                  tusb_control_request_t const *request)
{
  if(stage != CONTROL_STAGE_SETUP) {
    return true;
  }

  /* Standard SET_INTERFACE during enumeration. */
  if(request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD
     && request->bRequest == TUSB_REQ_SET_INTERFACE
     && request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE) {
    tud_control_status(rhport, request);
    return true;
  }

  /*
   * Nordic's nrfutil and the Nordic SDK's own app_usbd_nrf_dfu_trigger
   * dispatch class-type requests purely by direction+type, ignoring the
   * recipient field. Tools in the wild (incl. the libusb-based scripts
   * documented at https://hackjumpzero.ca/posts/2024/04/trigger-dfu-via-usb-on-nrf52840-dongle/)
   * send DETACH with bmRequestType = OUT|CLASS|DEVICE (0x20), not the
   * INTERFACE recipient (0x21) that a strict USB spec reader would
   * expect. Accept any recipient here for compatibility.
   */
  if(request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR
     && request->bmRequestType_bit.type != TUSB_REQ_TYPE_CLASS) {
    return false;
  }

  switch(request->bRequest) {
  case NORDIC_DFU_TRIGGER_REQ_DETACH:
    /* Drive the dongle's self-reset GPIO (P0.19, solder-bridged to
     * nRESET via SB2 on PCA10059) immediately, exactly like the OLD
     * arch/cpu/nrf52840/usb/usb-dfu-trigger.c handler. Once the pin
     * is driven low, the hardware reset fires within microseconds and
     * the bootloader sees RESETREAS.RESETPIN set, which it treats as
     * a request to enter DFU mode. No need to ACK the control transfer
     * or do any further work -- the chip is gone. */
    nrf_gpio_cfg_output(BOARD_DFU_SELF_RESET_PIN);
    nrf_gpio_pin_clear(BOARD_DFU_SELF_RESET_PIN);
    return true;
  case NORDIC_DFU_TRIGGER_REQ_NORDIC_INFO:
    return tud_control_xfer(rhport, request,
                            (void *)&nordic_info, sizeof(nordic_info));
  case NORDIC_DFU_TRIGGER_REQ_SEM_VER:
    return tud_control_xfer(rhport, request,
                            (void *)nordic_sem_ver,
                            sizeof(nordic_sem_ver) - 1);
  default:
    return false;
  }
}
/*---------------------------------------------------------------------------*/
static usbd_class_driver_t const nordic_dfu_trigger_driver = {
#if CFG_TUSB_DEBUG >= 2
  .name = "NORDIC-DFU",
#endif
  .init = nordic_dfu_trigger_init,
  .reset = nordic_dfu_trigger_reset,
  .open = nordic_dfu_trigger_open,
  .control_xfer_cb = nordic_dfu_trigger_control_xfer_cb,
  .xfer_cb = NULL,
  .sof = NULL,
};
/*---------------------------------------------------------------------------*/
/*
 * Register the Nordic DFU trigger driver with TinyUSB. TinyUSB picks
 * this up automatically via its weakly-defined application driver hook.
 */
usbd_class_driver_t const *
usbd_app_driver_get_cb(uint8_t *driver_count)
{
  *driver_count = 1;
  return &nordic_dfu_trigger_driver;
}
/*---------------------------------------------------------------------------*/
/*
 * Nordic's nrfutil (and the open-source Python triggers that mimic it)
 * send the DFU DETACH control request with bmRequestType recipient
 * field set to DEVICE rather than INTERFACE. The Nordic SDK's
 * app_usbd_nrf_dfu_trigger never checks the recipient field, so it
 * accepts the request regardless. TinyUSB, on the other hand, routes
 * INTERFACE-recipient class requests to per-interface class drivers
 * (which is what nordic_dfu_trigger_control_xfer_cb above handles) but
 * routes DEVICE-recipient class requests through this global hook.
 *
 * Mirror Nordic's behavior by catching the DETACH request here too, so
 * either recipient form works.
 */
bool
tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                          tusb_control_request_t const *request)
{
  if(stage != CONTROL_STAGE_SETUP) {
    return true;
  }

  /* Only catch Nordic's vendor-class control requests at device level.
   * Standard requests at device level (descriptors, configuration, etc.)
   * are TinyUSB's responsibility. */
  if(request->bmRequestType_bit.type != TUSB_REQ_TYPE_CLASS
     && request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR) {
    return false;
  }

  switch(request->bRequest) {
  case NORDIC_DFU_TRIGGER_REQ_DETACH:
    /* Same inline pin-reset as the standard runtime callback above. */
#if BOARD_DFU_SELF_RESET_PIN >= 0
    nrf_gpio_cfg_output(BOARD_DFU_SELF_RESET_PIN);
    nrf_gpio_pin_clear(BOARD_DFU_SELF_RESET_PIN);
#endif
    nrf_power_gpregret_set(NRF_POWER, 0, BOOTLOADER_DFU_START);
    NVIC_SystemReset();
    return true;
  case NORDIC_DFU_TRIGGER_REQ_NORDIC_INFO:
    return tud_control_xfer(rhport, request,
                            (void *)&nordic_info, sizeof(nordic_info));
  case NORDIC_DFU_TRIGGER_REQ_SEM_VER:
    return tud_control_xfer(rhport, request,
                            (void *)nordic_sem_ver,
                            sizeof(nordic_sem_ver) - 1);
  default:
    return false;
  }
}
/*---------------------------------------------------------------------------*/
#endif /* NRF_HAS_USB */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */

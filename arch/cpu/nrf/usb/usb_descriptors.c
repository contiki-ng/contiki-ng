/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )

/* Use Nordic's blessed VID:PID for the nRF52840 dongle CDC + DFU-trigger
 * image (0x1915:0x520F). nrfutil v7+ keys its `nordicDfu` trait
 * detection off this exact pair, so a vendor-specific DFU trigger
 * interface only gets exercised by the host when the device announces
 * itself with this PID. This matches what the OLD arch/platform/nrf52840
 * port did via APP_USBD_PID in sdk_config.h. */
#ifndef USB_PID
#define USB_PID  0x520Fu
#endif

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,

    // Use Interface Association Descriptor (IAD) for CDC
    // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0x1915,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum
{
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
#if CFG_TUD_DFU_RUNTIME
  ITF_NUM_DFU_RT,
#endif
  ITF_NUM_NORDIC_DFU,
  ITF_NUM_TOTAL
};

/* Length of the Nordic-vendor DFU trigger interface descriptor block:
 * one 9-byte interface descriptor + one 9-byte Nordic functional descriptor. */
#define NORDIC_DFU_TRIGGER_DESC_LEN (9 + 9)

#if CFG_TUD_DFU_RUNTIME
#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN \
                             + TUD_DFU_RT_DESC_LEN + NORDIC_DFU_TRIGGER_DESC_LEN)
#else
#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN \
                             + NORDIC_DFU_TRIGGER_DESC_LEN)
#endif

/* Raw bytes for the Nordic DFU trigger interface + functional descriptor.
 * Mirrors what arch/cpu/nrf52840/lib/nrf52-sdk/.../app_usbd_nrf_dfu_trigger
 * emits so nrfutil can recognise the interface. */
#define NORDIC_DFU_TRIGGER_DESCRIPTOR(_itfnum, _stridx) \
  /* Interface */ \
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 0, 0xFF, 0x01, 0x01, _stridx, \
  /* Functional (bmAttributes = bitCanDnload | bitWillDetach,
   *              wDetachTimeout = 1000, wTransferSize = 4096,
   *              bcdDFUVersion = 0x0101) */ \
  9, 0x21, 0x09, 0xE8, 0x03, 0x00, 0x10, 0x01, 0x01

#if CFG_TUSB_MCU == OPT_MCU_LPC175X_6X || CFG_TUSB_MCU == OPT_MCU_LPC177X_8X || CFG_TUSB_MCU == OPT_MCU_LPC40XX
  // LPC 17xx and 40xx endpoint type (bulk/interrupt/iso) are fixed by its number
  // 0 control, 1 In, 2 Bulk, 3 Iso, 4 In, 5 Bulk etc ...
  #define EPNUM_CDC_NOTIF   0x81
  #define EPNUM_CDC_OUT     0x02
  #define EPNUM_CDC_IN      0x82

#elif CFG_TUSB_MCU == OPT_MCU_SAMG
  // SAMG doesn't support a same endpoint number with different direction IN and OUT
  //    e.g EP1 OUT & EP1 IN cannot exist together
  #define EPNUM_CDC_NOTIF   0x81
  #define EPNUM_CDC_OUT     0x02
  #define EPNUM_CDC_IN      0x83

#elif CFG_TUSB_MCU == OPT_MCU_CXD56
  // CXD56 doesn't support a same endpoint number with different direction IN and OUT
  //    e.g EP1 OUT & EP1 IN cannot exist together
  // CXD56 USB driver has fixed endpoint type (bulk/interrupt/iso) and direction (IN/OUT) by its number
  // 0 control (IN/OUT), 1 Bulk (IN), 2 Bulk (OUT), 3 In (IN), 4 Bulk (IN), 5 Bulk (OUT), 6 In (IN)
  #define EPNUM_CDC_NOTIF   0x83
  #define EPNUM_CDC_OUT     0x02
  #define EPNUM_CDC_IN      0x81

#else
  #define EPNUM_CDC_NOTIF   0x81
  #define EPNUM_CDC_OUT     0x02
  #define EPNUM_CDC_IN      0x82

#endif

/* DFU runtime descriptor parameters: bitWillDetach so the device reboots
 * to bootloader on its own when the host sends a DETACH request. The
 * detach timeout and transfer size are conventional values; the actual
 * download happens after the device has re-enumerated as the bootloader,
 * so the runtime transfer size is not load-bearing. */
#define DFU_RT_ATTR     (DFU_ATTR_CAN_DOWNLOAD | DFU_ATTR_WILL_DETACH)
#define DFU_RT_TIMEOUT  1000
#define DFU_RT_XFER     4096

uint8_t const desc_fs_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

  // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
#if CFG_TUD_DFU_RUNTIME
  TUD_DFU_RT_DESCRIPTOR(ITF_NUM_DFU_RT, 5, DFU_RT_ATTR, DFU_RT_TIMEOUT, DFU_RT_XFER),
#endif
  NORDIC_DFU_TRIGGER_DESCRIPTOR(ITF_NUM_NORDIC_DFU, 6),
};

#if TUD_OPT_HIGH_SPEED
uint8_t const desc_hs_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

  // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 512),
#if CFG_TUD_DFU_RUNTIME
  TUD_DFU_RT_DESCRIPTOR(ITF_NUM_DFU_RT, 5, DFU_RT_ATTR, DFU_RT_TIMEOUT, DFU_RT_XFER),
#endif
  NORDIC_DFU_TRIGGER_DESCRIPTOR(ITF_NUM_NORDIC_DFU, 6),
};
#endif


// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index; // for multiple configurations

#if TUD_OPT_HIGH_SPEED
  // Although we are highspeed, host may be fullspeed.
  return (tud_speed_get() == TUSB_SPEED_HIGH) ?  desc_hs_configuration : desc_fs_configuration;
#else
  return desc_fs_configuration;
#endif
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const* string_desc_arr [] =
{
  (const char[]) { 0x09, 0x04 },  // 0: is supported language is English (0x0409)
  NULL,                           // 1: Manufacturer
  NULL,                           // 2: Product
  NULL,                           // 3: Serials, should use chip ID
  NULL,                           // 4: CDC Interface
#if CFG_TUD_DFU_RUNTIME
  "Contiki-NG DFU",               // 5: DFU runtime Interface
#else
  NULL,                           // 5: (placeholder, keep index alignment)
#endif
  "Contiki-NG Nordic DFU",        // 6: Nordic DFU trigger Interface
};

void usb_descriptor_set_manufacturer(char * manufacturer)
{
  string_desc_arr[1] = manufacturer;
}

void usb_descriptor_set_product(char * product)
{
  string_desc_arr[2] = product;
}

void usb_descriptor_set_serial(char * serial)
{
  string_desc_arr[3] = serial;
}

void usb_descriptor_set_cdc_interface(char * cdc_interface)
{
  string_desc_arr[4] = cdc_interface;
}

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;

  uint8_t chr_count;

  if ( index == 0)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }else
  {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;

    const char* str = string_desc_arr[index];

    // Cap at max char
    chr_count = strlen(str);
    if ( chr_count > 31 ) chr_count = 31;

    // Convert ASCII string into UTF-16
    for(uint8_t i=0; i<chr_count; i++)
    {
      _desc_str[1+i] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

  return _desc_str;
}

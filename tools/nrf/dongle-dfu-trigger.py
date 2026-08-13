#!/usr/bin/env python3
#
# Copyright (C) 2026 RISE Research Institutes of Sweden AB
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# SPDX-License-Identifier: BSD-3-Clause
#
"""
Trigger Nordic open-bootloader DFU mode on a PCA10059 dongle running
Contiki-NG firmware with USB DFU trigger support.

Sends the Nordic-vendor-specific DETACH control transfer that the
firmware's USB DFU trigger interface listens for, the same way the OLD
arch/cpu/nrf52840/usb/usb-dfu-trigger.c handler expected.

  bmRequestType = OUT | CLASS | DEVICE   (= 0x20)
  bRequest      = 0x00                   (DETACH)
  wValue        = 0
  wIndex        = 3                      (Contiki-NG's Nordic-DFU
                                          interface number; matches
                                          our usb_descriptors.c layout)
  wLength       = 0

The firmware receives this in nordic_dfu_trigger_control_xfer_cb() in
arch/cpu/nrf/dev/usb-arch.c, which then drives the dongle's self-reset
GPIO (P0.19) low to pin-reset the chip into the bootloader.

Usage:
  dongle-dfu-trigger.py                  # the single attached dongle
  dongle-dfu-trigger.py --serial-number <SN>
                                         # restrict to a specific dongle
  dongle-dfu-trigger.py --list           # list serial numbers of all
                                         # attached dongles and exit

Exits 0 on success or "device already in bootloader" (idempotent),
1 on a hard error: no dongle is attached (in firmware or bootloader
mode), the detach could not be delivered, or - with --list - an
attached dongle cannot be addressed by its serial number. Exits 2 when
several dongles are attached and no serial number was given, 3 when
the dongle state cannot be checked at all (pyusb or its libusb backend
is not installed; the caller may choose to assume bootloader mode).
"""
import argparse
import errno
import os
import sys

try:
    import usb.core
    import usb.util
except ImportError:
    print("error: pyusb not installed (run: pip install pyusb)", file=sys.stderr)
    sys.exit(3)

VID = 0x1915
APP_PID = 0x520F   # Contiki-NG dongle app w/ DFU trigger
BL_PID = 0x521F    # Nordic open-bootloader in DFU mode


def _usb_find(**kwargs):
    """usb.core.find, reporting a missing libusb backend the same way
    as a missing pyusb: the dongle state cannot be checked (exit 3)."""
    try:
        return usb.core.find(**kwargs)
    except usb.core.NoBackendError:
        print("error: no usb backend (libusb) available", file=sys.stderr)
        sys.exit(3)


def _get_serial(dev):
    try:
        return usb.util.get_string(dev, dev.iSerialNumber)
    except (usb.core.USBError, ValueError):
        return None


def _match_serial(serial):
    """Return a pyusb matcher that selects only devices whose USB
    iSerialNumber descriptor equals ``serial``. ``None`` selects any."""
    if serial is None:
        return lambda d: True
    return lambda d: _get_serial(d) == serial


def _find_dongles():
    """Return all attached dongles, in firmware or bootloader mode, as
    a list of (device, pid) tuples."""
    return [
        (dev, pid)
        for pid in (APP_PID, BL_PID)
        for dev in _usb_find(find_all=True, idVendor=VID, idProduct=pid) or []
    ]


parser = argparse.ArgumentParser(description=__doc__.splitlines()[1])
parser.add_argument(
    "--serial-number",
    help="Restrict the trigger to the dongle with this USB serial number.",
)
parser.add_argument(
    "--list", action="store_true",
    help="List the serial numbers of all attached dongles and exit.",
)
args = parser.parse_args()

if args.list:
    # Every attached dongle must be addressable by a unique serial
    # number, or the caller would silently program only a subset.
    all_serials = [_get_serial(dev) for dev, _ in _find_dongles()]
    if not all_serials:
        print("error: no dongle found", file=sys.stderr)
        sys.exit(1)
    serials = [s for s in all_serials if s is not None]
    if len(serials) < len(all_serials):
        print(f"error: cannot read the serial number of "
              f"{len(all_serials) - len(serials)} of {len(all_serials)} "
              f"attached dongles", file=sys.stderr)
        print("    (often insufficient USB permissions; see the udev rule\n"
              "     in doc/platforms/nrf.md)", file=sys.stderr)
        sys.exit(1)
    duplicates = sorted({s for s in serials if serials.count(s) > 1})
    if duplicates:
        print("error: several dongles share a serial number: "
              + ", ".join(duplicates), file=sys.stderr)
        sys.exit(1)
    print("\n".join(sorted(serials)))
    sys.exit(0)

match = _match_serial(args.serial_number)

# Refuse to pick an arbitrary dongle: when no serial number is given and
# more than one candidate dongle is attached (in firmware or bootloader
# mode), the caller must select one. The serial number is preserved
# across the firmware/bootloader mode switch.
if args.serial_number is None:
    candidates = _find_dongles()
    if len(candidates) > 1:
        print("error: more than one dongle is attached:", file=sys.stderr)
        for dev, pid in candidates:
            serial = _get_serial(dev) or "<unknown serial>"
            mode = "bootloader" if pid == BL_PID else "firmware"
            print(f"  {serial} ({mode} mode)", file=sys.stderr)
        print("select one with --serial-number (make NRF_UPLOAD_SN=<serial>)",
              file=sys.stderr)
        sys.exit(2)

# If the bootloader is already enumerated for our target, there's nothing to do.
if _usb_find(idVendor=VID, idProduct=BL_PID, custom_match=match) is not None:
    print("Dongle already in DFU bootloader mode.")
    sys.exit(0)

dev = _usb_find(idVendor=VID, idProduct=APP_PID, custom_match=match)
if dev is None:
    msg = (f"no dongle with VID:PID {VID:04X}:{APP_PID:04X} (firmware) or "
           f"{VID:04X}:{BL_PID:04X} (bootloader)")
    if args.serial_number:
        msg += f" and serial number {args.serial_number}"
    print(f"error: {msg} found", file=sys.stderr)
    print("    (a dongle running firmware without DFU trigger support can be\n"
          "     put in bootloader mode by pressing its RESET button)",
          file=sys.stderr)
    sys.exit(1)

bmRequestType = usb.util.build_request_type(
    usb.util.CTRL_OUT,
    usb.util.CTRL_TYPE_CLASS,
    usb.util.CTRL_RECIPIENT_DEVICE,
)

try:
    dev.ctrl_transfer(bmRequestType, 0x00, 0, 3, None, timeout=1000)
except usb.core.USBError as e:
    # Expected: the chip pin-resets in the middle of the control
    # transfer. A permission or busy error, however, means the detach
    # was never delivered.
    if e.errno in (errno.EACCES, errno.EPERM, errno.EBUSY):
        print(f"error: cannot send DFU detach request: {e}", file=sys.stderr)
        sys.exit(1)

print("DFU detach triggered.", flush=True)
# The dongle has just dropped off the bus, and libusb (notably its macOS
# IOKit backend) can hang while closing the vanished device handle
# during interpreter teardown. Exit without running any finalizers.
os._exit(0)

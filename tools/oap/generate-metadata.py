#!/usr/bin/env python

# Copyright (c) 2018, George Oikonomou - http://www.spd.gr
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
import crcmod
import os
import argparse
import subprocess
import random
import struct

try:
    import magic
    magic.from_file
    have_magic = True
except (ImportError, AttributeError):
    have_magic = False

try:
    from intelhex import IntelHex
    have_hex_support = True
except ImportError:
    have_hex_support = False

version_string = '0.1-beta'


class GenerateMetadataException(Exception):
    pass


class FirmwareFile(object):
    HEX_FILE_EXTENSIONS = ('hex', 'ihx', 'ihex')

    def __init__(self, path, build, out_hex, offset, big_endian=False):
        self.build = build
        self._out_hex = out_hex
        self._offset = offset
        self.big_endian = big_endian
        self.random = random.randint(0, 0xFFFFFFFF)
        firmware_is_hex = False

        if have_magic:
            file_type = bytearray(magic.from_file(path, True))

            # from_file() returns bytes with PY3, str with PY2. This comparison
            # will be True in both cases"""
            if file_type == b'text/plain':
                firmware_is_hex = True
                print('Firmware file: Intel Hex')
            elif file_type == b'application/octet-stream':
                print('Firmware file: Raw Binary')
            else:
                error_str = "Could not determine firmware type. Magic " \
                            "indicates '%s'" % (file_type,)
                raise GenerateMetadataException(error_str)
        else:
            if os.path.splitext(path)[1][1:] in self.HEX_FILE_EXTENSIONS:
                firmware_is_hex = True
                print("Your firmware looks like an Intel Hex file")
            else:
                print('Cannot auto-detect firmware filetype: Assuming .bin')

            print('For more solid firmware type auto-detection, install '
                  'python-magic.')

        if firmware_is_hex:
            if have_hex_support:
                self.bytes = bytearray(IntelHex(path).tobinarray())
                return
            else:
                error_str = "Firmware is Intel Hex, but the IntelHex library " \
                            "could not be imported.\n" \
                            "Install IntelHex in site-packages or program " \
                            "your device with a raw binary (.bin) file.\n" \
                            "Please see the readme for more details."
                raise GenerateMetadataException(error_str)

        with open(path, 'rb') as f:
            self.bytes = bytearray(f.read())

    def crc(self):
        crc16 = crcmod.mkCrcFun(0x11021, rev=True, initCrc=0x0000,
                                xorOut=0x0000)

        # self.bytes is a bytearray. Hopefully bytes(self.bytes) is portable
        # across python versions

        return(crc16(bytes(self.bytes)))

    def length(self):
        return len(self.bytes)

    def save_hex(self):
        ord_char = '>' if self.big_endian else '<'
        out_fmt = ord_char + 'LLHH'
        packed = struct.pack(out_fmt, self.length(), self.random,
                             self.crc(), self.build)

        oh = IntelHex()
        oh.frombytes(bytearray(packed), offset=self._offset)
        oh.write_hex_file(self._out_hex)


def print_version():
    git_describe_cmd = ['git', 'describe', '--tags', '--always']
    try:
        # In 2.7 check_output returns string, in 3+ it returns bytes.
        # This should be portable.
        version = subprocess.check_output(git_describe_cmd).decode('utf-8')
    except (IOError, OSError, subprocess.CalledProcessError):
        version = version_string
    return version


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False,
                                     description='Calculate OAP metadata for '
                                                 'a Contiki-NG firmware')
    parser.add_argument('firmware', help='Path to firmware file')
    parser.add_argument('-b', '--build', action = 'store', type = int,
                          default = 0,
                          help = 'Set firmware build number (Default: 0)')
    parser.add_argument('-B', '--big-endian', action = 'store_true',
                        default = False,
                        help = 'Save metadata in big-endian format. '
                               'Only makes sense with -o')
    parser.add_argument('-o', '--out-file', action = 'store', nargs = '?',
                        const = 'metadata.hex', default = False,
                        help = 'Save the metadata in OUT_FILE. \
                                If -o is specified but OUT_FILE is omitted, \
                                metadata.hex will be used. If the argument is \
                                omitted altogether, metadata will not \
                                be saved.')
    parser.add_argument('-O', '--offset', action = 'store',
                        type = lambda x: int(x, 0),
                        default = 0,
                        help = 'Save metadata at offset OFFSET. Only makes '
                               'sense with -o')
    parser.add_argument('-h', '--help', action='help',
                        help='Show this message and exit')
    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s ' + print_version(),
                        help='Prints software version')

    args = parser.parse_args()

    firmware = FirmwareFile(args.firmware, args.build, args.out_file,
                            args.offset, args.big_endian)

    if args.out_file is not False:
        firmware.save_hex()

    print("Length=%u bytes, CRC16=0x%04x, ID=0x%08x, Build=%u"
          % (firmware.length(), firmware.crc(), firmware.random,
             firmware.build))

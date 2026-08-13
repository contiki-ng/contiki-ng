"""
Serial Radio Tools Package.

Provides Python utilities for controlling a Contiki-NG serial radio node.
"""

from .serial_radio import SerialRadio, SerialRadioError, RxFrame, ScanResult, list_serial_ports
from .protocol import Command, Event, ErrorCode, RadioParam, Key
from .slip import SlipDecoder, slip_encode, slip_decode
from .crc16 import crc16_data, crc16_verify, crc16_append

__all__ = [
    # Main API
    'SerialRadio',
    'SerialRadioError',
    'RxFrame',
    'ScanResult',
    'list_serial_ports',

    # Protocol constants
    'Command',
    'Event',
    'ErrorCode',
    'RadioParam',
    'Key',

    # Low-level utilities
    'SlipDecoder',
    'slip_encode',
    'slip_decode',
    'crc16_data',
    'crc16_verify',
    'crc16_append',
]

__version__ = '1.0.0'

"""
Serial Radio Protocol definitions.

Defines command opcodes, event types, and CBOR key mappings
that match the C implementation.
"""

from enum import IntEnum
from typing import Dict, Any


class Command(IntEnum):
    """Command opcodes (PC -> Node)."""
    PING = 0
    GET_PARAM = 1
    SET_PARAM = 2
    RSSI_SCAN_START = 3
    RSSI_SCAN_STOP = 4
    RX_ON = 5
    RX_OFF = 6
    FAST_SCAN_START = 7
    FAST_SCAN_STOP = 8
    JAM_START = 9
    JAM_STOP = 10
    TX_RAW_FRAME = 50


class Event(IntEnum):
    """Event opcodes (Node -> PC)."""
    PONG = 100
    PARAM_RESPONSE = 51
    RX_FRAME = 52
    RSSI_SCAN_RESULT = 53
    TX_RESPONSE = 54
    HEARTBEAT = 55
    FAST_SCAN_RESULT = 56
    ERROR = 255


class ErrorCode(IntEnum):
    """Error codes."""
    NONE = 0
    INVALID_CMD = 1
    INVALID_PARAM = 2
    CRC_FAIL = 3
    CBOR_DECODE = 4
    RADIO_ERROR = 5
    BUFFER_OVERFLOW = 6
    SCAN_ACTIVE = 7


class RadioParam(IntEnum):
    """
    Radio parameter codes.

    These match Contiki-NG's radio_param_e enum from os/dev/radio.h
    """
    POWER_MODE = 0
    CHANNEL = 1
    PAN_ID = 2
    SHORT_ADDR = 3  # 16BIT_ADDR
    RX_MODE = 4
    TX_MODE = 5
    TXPOWER = 6
    CCA_THRESHOLD = 7
    RSSI = 8
    LAST_RSSI = 9
    LAST_LINK_QUALITY = 10
    LAST_PACKET_TIMESTAMP = 11
    LONG_ADDR = 12  # 64BIT_ADDR
    SHR_SEARCH = 13
    IQ_LSBS = 14

    # Read-only constants (offset by 0x100)
    CHANNEL_MIN = 0x100
    CHANNEL_MAX = 0x101
    TXPOWER_MIN = 0x102
    TXPOWER_MAX = 0x103
    PHY_OVERHEAD = 0x104
    BYTE_AIR_TIME = 0x105
    DELAY_BEFORE_TX = 0x106
    DELAY_BEFORE_RX = 0x107
    DELAY_BEFORE_DETECT = 0x108
    MAX_PAYLOAD_LEN = 0x109


# CBOR map keys (single character for compactness)
class Key:
    """CBOR map key constants."""
    TYPE = 't'      # Message type/opcode
    ID = 'i'        # Message ID for request/response matching
    PARAM = 'p'     # Radio parameter code
    VALUE = 'v'     # Parameter value
    FRAME = 'f'     # Raw radio frame data
    RSSI = 'r'      # RSSI value
    LQI = 'l'       # Link quality indicator
    CHANNEL = 'c'   # Channel number
    START_CH = 's'  # Scan start channel
    END_CH = 'e'    # Scan end channel
    DWELL = 'd'     # Scan dwell time (ms)
    ERROR = 'x'     # Error code
    VERSION = 'V'   # Version string
    RSSI_ARRAY = 'R'  # Array of RSSI values for fast scan
    SEQ = 'n'       # Sequence number


# Human-readable names for radio parameters
PARAM_NAMES: Dict[int, str] = {
    RadioParam.POWER_MODE: "Power Mode",
    RadioParam.CHANNEL: "Channel",
    RadioParam.PAN_ID: "PAN ID",
    RadioParam.SHORT_ADDR: "Short Address",
    RadioParam.RX_MODE: "RX Mode",
    RadioParam.TX_MODE: "TX Mode",
    RadioParam.TXPOWER: "TX Power (dBm)",
    RadioParam.CCA_THRESHOLD: "CCA Threshold",
    RadioParam.RSSI: "RSSI",
    RadioParam.LAST_RSSI: "Last RSSI",
    RadioParam.LAST_LINK_QUALITY: "Last LQI",
    RadioParam.CHANNEL_MIN: "Channel Min",
    RadioParam.CHANNEL_MAX: "Channel Max",
    RadioParam.TXPOWER_MIN: "TX Power Min",
    RadioParam.TXPOWER_MAX: "TX Power Max",
    RadioParam.MAX_PAYLOAD_LEN: "Max Payload Length",
}


def get_error_message(code: int) -> str:
    """Get human-readable error message."""
    messages = {
        ErrorCode.NONE: "Success",
        ErrorCode.INVALID_CMD: "Invalid command",
        ErrorCode.INVALID_PARAM: "Invalid parameter",
        ErrorCode.CRC_FAIL: "CRC check failed",
        ErrorCode.CBOR_DECODE: "CBOR decode error",
        ErrorCode.RADIO_ERROR: "Radio error",
        ErrorCode.BUFFER_OVERFLOW: "Buffer overflow",
        ErrorCode.SCAN_ACTIVE: "Scan already active",
    }
    return messages.get(code, f"Unknown error ({code})")

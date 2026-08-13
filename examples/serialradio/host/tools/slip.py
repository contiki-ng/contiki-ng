"""
SLIP (Serial Line Internet Protocol) encoder/decoder.

This module provides SLIP framing for serial communication.
RFC 1055 compliant with extensions for debugging.
"""

from typing import Optional, Tuple, List
from dataclasses import dataclass
from enum import IntEnum


class SlipBytes(IntEnum):
    """SLIP special bytes."""
    END = 0xC0
    ESC = 0xDB
    ESC_END = 0xDC
    ESC_ESC = 0xDD


@dataclass
class SlipDecoder:
    """
    SLIP frame decoder with support for debug text extraction.

    Separates SLIP-framed data from plain text debug output.
    """

    def __init__(self):
        self._buffer: bytearray = bytearray()
        self._in_frame: bool = False
        self._escape: bool = False
        self._debug_buffer: bytearray = bytearray()

    def _is_valid_cbor_frame(self, data: bytes) -> bool:
        """
        Check if data looks like a valid CBOR frame vs debug text.

        Valid CBOR messages from the device start with a map.
        Debug text typically starts with printable ASCII like '['.
        """
        if len(data) < 3:  # Minimum: 1 byte CBOR + 2 byte CRC
            return False

        first_byte = data[0]

        # CBOR map types: 0xA0-0xBF (small maps) or 0xBF (indefinite map)
        if (first_byte >= 0xA0 and first_byte <= 0xBF):
            return True

        # Also accept 0xB9-0xBB for larger maps (16/32/64-bit length)
        if first_byte in (0xB9, 0xBA, 0xBB):
            return True

        # If it starts with printable ASCII (especially '[' for LOG output),
        # it's definitely debug text that got mixed into frame buffer
        if first_byte >= 0x20 and first_byte < 0x7F:
            return False

        return True  # Assume valid for other cases

    def feed(self, data: bytes) -> Tuple[List[bytes], str]:
        """
        Feed raw bytes into the decoder.

        Args:
            data: Raw bytes from serial port

        Returns:
            Tuple of (list of complete SLIP frames, debug text)
        """
        frames: List[bytes] = []

        for byte in data:
            if byte == SlipBytes.END:
                if self._in_frame and len(self._buffer) > 0:
                    # Check if this looks like a valid CBOR frame or debug text
                    frame_data = bytes(self._buffer)
                    if self._is_valid_cbor_frame(frame_data):
                        frames.append(frame_data)
                    else:
                        # Treat as debug text that got captured during framing
                        try:
                            debug_str = frame_data.decode('utf-8', errors='replace')
                            self._debug_buffer.extend(debug_str.encode('utf-8'))
                        except:
                            pass  # Ignore decode errors
                # Reset for next frame
                self._buffer.clear()
                self._in_frame = True
                self._escape = False
            elif not self._in_frame:
                # Data outside SLIP frame -> debug text
                if byte == ord('\n'):
                    self._debug_buffer.append(byte)
                elif byte >= 0x20 and byte < 0x7F:
                    self._debug_buffer.append(byte)
                elif byte == ord('\r'):
                    pass  # Ignore CR
            elif self._escape:
                self._escape = False
                if byte == SlipBytes.ESC_END:
                    self._buffer.append(SlipBytes.END)
                elif byte == SlipBytes.ESC_ESC:
                    self._buffer.append(SlipBytes.ESC)
                else:
                    # Protocol error - store as-is
                    self._buffer.append(byte)
            elif byte == SlipBytes.ESC:
                self._escape = True
            else:
                self._buffer.append(byte)

        # Extract debug text
        debug_text = self._debug_buffer.decode('utf-8', errors='replace')
        self._debug_buffer.clear()

        return frames, debug_text

    def reset(self):
        """Reset decoder state."""
        self._buffer.clear()
        self._in_frame = False
        self._escape = False
        self._debug_buffer.clear()


def slip_encode(data: bytes) -> bytes:
    """
    Encode data into a SLIP frame.

    Args:
        data: Raw data to encode

    Returns:
        SLIP-encoded frame with END delimiters
    """
    result = bytearray()
    result.append(SlipBytes.END)

    for byte in data:
        if byte == SlipBytes.END:
            result.append(SlipBytes.ESC)
            result.append(SlipBytes.ESC_END)
        elif byte == SlipBytes.ESC:
            result.append(SlipBytes.ESC)
            result.append(SlipBytes.ESC_ESC)
        else:
            result.append(byte)

    result.append(SlipBytes.END)
    return bytes(result)


def slip_decode(frame: bytes) -> Optional[bytes]:
    """
    Decode a single SLIP frame (without END delimiters).

    Args:
        frame: SLIP frame data (after stripping END bytes)

    Returns:
        Decoded data or None if invalid
    """
    result = bytearray()
    escape = False

    for byte in frame:
        if escape:
            escape = False
            if byte == SlipBytes.ESC_END:
                result.append(SlipBytes.END)
            elif byte == SlipBytes.ESC_ESC:
                result.append(SlipBytes.ESC)
            else:
                # Protocol error
                return None
        elif byte == SlipBytes.ESC:
            escape = True
        elif byte == SlipBytes.END:
            # Unexpected END in middle of frame
            return None
        else:
            result.append(byte)

    if escape:
        # Incomplete escape sequence
        return None

    return bytes(result)

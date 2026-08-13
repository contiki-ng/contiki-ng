"""
CRC-16 CCITT implementation.

Compatible with Contiki-NG's lib/crc16.c implementation.
"""


def crc16_add(byte: int, crc: int) -> int:
    """
    Update CRC with a single byte.

    This matches Contiki-NG's crc16_add() function from os/lib/crc16.c

    Args:
        byte: Byte value (0-255)
        crc: Current CRC value

    Returns:
        Updated CRC value
    """
    crc = crc & 0xFFFF
    byte = byte & 0xFF

    # Match Contiki-NG's crc16_add exactly:
    # acc ^= b;
    # acc  = (acc >> 8) | (acc << 8);
    # acc ^= (acc & 0xff00) << 4;
    # acc ^= (acc >> 8) >> 4;
    # acc ^= (acc & 0xff00) >> 5;
    crc ^= byte
    crc = ((crc >> 8) | (crc << 8)) & 0xFFFF
    crc ^= ((crc & 0xFF00) << 4) & 0xFFFF
    crc ^= (crc >> 8) >> 4
    crc ^= ((crc & 0xFF00) >> 5) & 0xFFFF

    return crc & 0xFFFF


def crc16_data(data: bytes, initial: int = 0) -> int:
    """
    Calculate CRC-16 over a block of data.

    Args:
        data: Data bytes
        initial: Initial CRC value (default 0)

    Returns:
        CRC-16 value
    """
    crc = initial & 0xFFFF

    for byte in data:
        crc = crc16_add(byte, crc)

    return crc


def crc16_verify(data: bytes) -> bool:
    """
    Verify data with appended CRC-16 (little-endian).

    Args:
        data: Data with 2-byte CRC appended (little-endian)

    Returns:
        True if CRC is valid
    """
    if len(data) < 2:
        return False

    payload = data[:-2]
    received_crc = data[-2] | (data[-1] << 8)
    computed_crc = crc16_data(payload)

    return received_crc == computed_crc


def crc16_append(data: bytes) -> bytes:
    """
    Append CRC-16 to data (little-endian).

    Args:
        data: Data bytes

    Returns:
        Data with 2-byte CRC appended
    """
    crc = crc16_data(data)
    return data + bytes([crc & 0xFF, (crc >> 8) & 0xFF])

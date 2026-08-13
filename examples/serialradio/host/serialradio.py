#!/usr/bin/env python3
"""
Serial Radio CLI launcher script.

Usage:
    uv run serial-radio /dev/ttyUSB0
    ./serialradio.py /dev/tty.usbserial-1234
"""

from tools.cli import main

if __name__ == '__main__':
    main()

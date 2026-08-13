#!/usr/bin/env python3
"""
Allow running the CLI as a module:
    python -m tools
    python -m tools.cli /dev/ttyUSB0
"""

from .cli import main

if __name__ == '__main__':
    main()

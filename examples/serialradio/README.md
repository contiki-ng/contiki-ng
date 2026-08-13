# Serial Radio Control Interface

A serial radio control interface for Contiki-NG nodes, providing spectrum analysis, packet sniffing, and radio parameter control over UART.

## Features

- **Radio Control**: Get/set channel, TX power, PAN ID, and other radio parameters
- **RSSI Scanning**: Single-channel and multi-channel spectrum scanning
- **Fast Scan**: Rapid all-channel RSSI sweeps for real-time spectrum visualization
- **Packet Sniffing**: Capture raw 802.15.4 frames with RSSI/LQI metadata
- **Frame Injection**: Send raw radio frames for testing
- **Jamming Mode**: Continuous transmission for interference testing
- **Web Interface**: Real-time spectrum visualization and control via browser
- **Python API**: High-level library for scripting and automation

## Protocol

Uses CBOR encoding over SLIP framing with CRC16 integrity checking:

```
SLIP frame: 0xC0 [CBOR message + CRC16] 0xC0
```

Debug output coexists with protocol messages on the same UART.

## Building

```bash
# For CC1352 LaunchPad (2.4 GHz)
make TARGET=simplelink BOARD=launchpad/cc1352r1

# For CC1352 SensorTag (Sub-GHz)
make TARGET=simplelink BOARD=sensortag/cc1352r1
```

## Python Tools

### Installation

The Python tools live in the `host/` subdirectory, packaged with a standard
`pyproject.toml`. The recommended way to run them is with
[uv](https://docs.astral.sh/uv/), which creates the virtual environment and
installs the dependencies (pyserial, cbor2, websockets) automatically:

```bash
cd host              # from examples/serialradio/
uv run serial-radio /dev/ttyACM0
```

Alternatively, install the dependencies into your own environment with pip:

```bash
cd host
pip install pyserial cbor2 websockets
python -m tools.cli /dev/ttyACM0
```

### CLI Usage

```bash
# from examples/serialradio/host/
uv run serial-radio /dev/ttyACM0   # or: python -m tools.cli /dev/ttyACM0
```

CLI Commands:
- `ping` - Test connection
- `info` - Show radio info and parameters
- `channel [N]` - Get/set channel
- `power [N]` - Get/set TX power (dBm)
- `rssi` - Get current RSSI reading
- `scan [start] [end] [dwell_ms]` - Single RSSI scan
- `fastscan start [start_ch] [end_ch]` - Start continuous fast scanning
- `fastscan stop` - Stop fast scanning
- `sniff [channel]` - Start packet sniffing
- `sniff stop` - Stop sniffing
- `rx on|off` - Enable/disable radio receiver
- `tx <hex>` - Transmit raw frame
- `jam start [channel] [interval_ms]` - Start jamming
- `jam stop` - Stop jamming
- `webserver [port]` - Start web interface (default: 8080)

### Web Interface

Start the web server (from `examples/serialradio/host/`):
```bash
uv run serial-radio /dev/ttyACM0   # or: python -m tools.cli /dev/ttyACM0
> webserver
```

Then open http://localhost:8080 in your browser.

Features:
- **CLI Tab**: Console output, command input, radio info
- **RSSI Scan Tab**: 2D bar chart and 3D waterfall spectrum display
- **Packet Sniffer Tab**: Live packet capture with hex display

### Python API

Run from `examples/serialradio/host/` (e.g. `uv run python your_script.py`):

```python
from tools import SerialRadio, RadioParam

radio = SerialRadio('/dev/ttyACM0')
radio.connect()

# Get/set parameters
channel = radio.get_channel()
radio.set_channel(26)
radio.set_tx_power(0)

# Packet sniffing
radio.set_rx_callback(lambda frame: print(f"RX: {frame.data.hex()}"))
radio.rx_on()

# Fast scanning
radio.set_fast_scan_callback(lambda scan: print(f"RSSI: {scan.rssi_values}"))
radio.start_fast_scan(11, 26)

radio.disconnect()
```

## Channel Ranges

| Band | Region | Channels |
|------|--------|----------|
| 2.4 GHz | Worldwide | 11-26 |
| 863 MHz | Europe | 0-33 |
| 915 MHz | US | 0-128 |
| 920 MHz | Japan | 0-37 |

## Files

```
serialradio/
├── serial-radio.c          # Firmware: main implementation + autostart
├── serial-radio.h          # Firmware: protocol definitions
├── sniffer-mac.c           # Firmware: MAC driver for sniffing
├── Makefile
├── project-conf.h
└── host/                   # Python host tools (run on your PC)
    ├── pyproject.toml      # Packaging (uv)
    ├── uv.lock             # Locked dependencies
    ├── serialradio.py      # Launcher
    └── tools/              # The 'tools' Python package
        ├── __init__.py
        ├── cli.py          # Interactive CLI
        ├── serial_radio.py # Python API
        ├── webserver.py    # Web interface server
        ├── protocol.py     # Protocol constants
        ├── slip.py         # SLIP encoder/decoder
        ├── crc16.py        # CRC16 implementation
        └── www/
            ├── index.html  # Web UI
            └── spectrum.js # Visualization
```

## Use as a border-router radio

The native RPL border router (`examples/rpl-border-router`, `TARGET=native`) can
optionally use a serialradio node as its 802.15.4 radio over the CBOR protocol,
instead of the default legacy ASCII `slip-radio` protocol. Enable it at build
time with `BORDER_ROUTER_SERIAL_RADIO=1`. The border router then queries the
radio's EUI-64 (`GET_ADDR64`), adopts it as its own link-layer address, sets the
PAN ID / channel, and enables a border-router *router mode* (`ROUTER_MODE`) that
turns on hardware address filtering and auto-ACK so unicast traffic to the
router is received and acknowledged.

```bash
# 1. Build + flash the serialradio firmware (2.4 GHz CC2538 on a Zoul Firefly)
cd examples/serialradio
make TARGET=zoul BOARD=firefly serial-radio.upload PORT=/dev/ttyUSB0

# 2. IMPORTANT: reset the board so it runs the application.
#    After flashing, cc2538 boards stay in the ROM bootloader (silent) until
#    a manual RESET / power-cycle.

# 3. Build the border router with serialradio (CBOR) support and run it
#    against the radio's serial port
cd ../rpl-border-router
make TARGET=native BORDER_ROUTER_SERIAL_RADIO=1 border-router
sudo ./build/native/border-router.native -s /dev/ttyUSB0 fd00::1/64
```

Look for the border router adopting the radio's address (e.g.
`fd00::212:4b00:9df:90a1`); nodes on the same PAN/channel then join the DAG and
become reachable through the host.

The radio's serial port is exclusive: run **either** the web/scan tools **or**
the border router at a time, not both. For border-router use, the firmware's
verbose `LOG_CONF_LEVEL_*` settings in `project-conf.h` may be lowered to reduce
debug text interleaved with protocol frames.

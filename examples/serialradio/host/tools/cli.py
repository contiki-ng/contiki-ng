#!/usr/bin/env python3
"""
Serial Radio Command-Line Interface.

Interactive tool for controlling a Contiki-NG serial radio node.

Usage:
    python -m tools.cli /dev/ttyUSB0
    python -m tools.cli --help
"""

import argparse
import cmd
import sys
import time
import threading
from typing import Optional, List

from .serial_radio import SerialRadio, SerialRadioError, RxFrame, ScanResult, Heartbeat, FastScanResult, list_serial_ports
from .protocol import RadioParam, PARAM_NAMES

# Optional web server support
try:
    from .webserver import SerialRadioWebServer, check_dependencies as check_web_dependencies
    WEBSERVER_AVAILABLE = True
except ImportError:
    WEBSERVER_AVAILABLE = False


class SerialRadioCLI(cmd.Cmd):
    """Interactive command-line interface for serial radio control."""

    intro = """
================================================================================
  Serial Radio Control Interface
  Type 'help' or '?' for available commands. Type 'quit' to exit.
================================================================================
"""
    prompt = 'radio> '

    def __init__(self, radio: SerialRadio):
        super().__init__()
        self.radio = radio
        self._sniffing = False
        self._scanning = False
        self._scan_results: List[ScanResult] = []
        self._webserver: Optional['SerialRadioWebServer'] = None

    # -------------------------------------------------------------------------
    # Connection commands
    # -------------------------------------------------------------------------

    def do_ping(self, arg):
        """Test connectivity with the radio node."""
        if self.radio.ping():
            print(f"PONG received! Version: {self.radio.version}")
        else:
            print("No response from device")

    def do_info(self, arg):
        """Display comprehensive radio information."""
        print("\nRadio Information:")
        print("-" * 40)

        info = self.radio.get_radio_info()

        # Detect frequency band from channel range
        ch_min = info.get('channel_min')
        ch_max = info.get('channel_max')
        band_info = self._detect_frequency_band(ch_min, ch_max)

        if band_info:
            print(f"  Radio Mode: {band_info}")
            print()

        for key, value in info.items():
            name = key.replace('_', ' ').title()
            if 'power' in key and 'mode' not in key:
                print(f"  {name}: {value} dBm")
            elif 'addr' in key:
                print(f"  {name}: 0x{value:04X}")
            elif 'pan' in key:
                print(f"  {name}: 0x{value:04X}")
            elif key == 'rx_mode':
                # Decode RX mode flags
                flags = []
                if value == 0:
                    flags.append("PROMISCUOUS")
                else:
                    if value & 0x01:
                        flags.append("ADDR_FILTER")
                    if value & 0x02:
                        flags.append("AUTOACK")
                    if value & 0x04:
                        flags.append("POLL_MODE")
                mode_str = " | ".join(flags) if flags else "NONE"
                print(f"  {name}: 0x{value:02X} ({mode_str})")
            else:
                print(f"  {name}: {value}")

        print(f"\n  Version: {self.radio.version or 'Unknown'}")
        print()

        # Refresh the web UI info panel with the values we just read.
        self._push_radio_info(info)

    def _detect_frequency_band(self, ch_min: Optional[int], ch_max: Optional[int]) -> Optional[str]:
        """Detect frequency band from channel range."""
        if ch_min is None or ch_max is None:
            return None

        # IEEE 802.15.4 2.4 GHz: channels 11-26
        if ch_min == 11 and ch_max == 26:
            return "IEEE 802.15.4 2.4 GHz (channels 11-26)"

        # IEEE 802.15.4g Sub-GHz bands (starting from channel 0)
        if ch_min == 0:
            if ch_max == 33:
                return "IEEE 802.15.4g Sub-GHz 863 MHz Europe (channels 0-33)"
            elif ch_max == 128 or ch_max == 129:
                return "IEEE 802.15.4g Sub-GHz 915 MHz US (channels 0-128)"
            elif ch_max == 37 or ch_max == 38:
                return "IEEE 802.15.4g Sub-GHz 920 MHz Japan (channels 0-37)"
            elif ch_max >= 100:
                return f"IEEE 802.15.4g Sub-GHz 915 MHz (channels 0-{ch_max})"
            elif ch_max >= 30:
                return f"IEEE 802.15.4g Sub-GHz 863/868 MHz (channels 0-{ch_max})"
            else:
                return f"Sub-GHz (channels 0-{ch_max})"

        # Unknown/custom range
        return f"Custom ({ch_min}-{ch_max} channels)"

    # -------------------------------------------------------------------------
    # Parameter commands
    # -------------------------------------------------------------------------

    def do_get(self, arg):
        """
        Get a radio parameter value.

        Usage: get <param>

        Parameters:
            channel     - Current radio channel
            power       - TX power in dBm
            rssi        - Current RSSI reading
            pan         - PAN ID
            addr        - Short address
            <number>    - Raw parameter number

        Example:
            get channel
            get rssi
            get 1
        """
        if not arg:
            print("Usage: get <param>")
            print("Parameters: channel, power, rssi, pan, addr, or numeric ID")
            return

        param = self._parse_param(arg)
        if param is None:
            print(f"Unknown parameter: {arg}")
            return

        value = self.radio.get_param(param)
        if value is not None:
            name = PARAM_NAMES.get(param, f"Param {param}")
            if param == RadioParam.TXPOWER:
                print(f"{name}: {value} dBm")
            elif param in (RadioParam.PAN_ID, RadioParam.SHORT_ADDR):
                print(f"{name}: 0x{value:04X}")
            else:
                print(f"{name}: {value}")
        else:
            print("Failed to get parameter")

    def do_set(self, arg):
        """
        Set a radio parameter value.

        Usage: set <param> <value>

        Parameters:
            channel <n>  - Set radio channel (11-26 for 802.15.4)
            power <dBm>  - Set TX power
            pan <id>     - Set PAN ID (hex or decimal)
            addr <addr>  - Set short address (hex or decimal)

        Example:
            set channel 26
            set power 5
            set pan 0xABCD
        """
        parts = arg.split()
        if len(parts) < 2:
            print("Usage: set <param> <value>")
            return

        param = self._parse_param(parts[0])
        if param is None:
            print(f"Unknown parameter: {parts[0]}")
            return

        try:
            value = self._parse_value(parts[1])
        except ValueError:
            print(f"Invalid value: {parts[1]}")
            return

        if self.radio.set_param(param, value):
            name = PARAM_NAMES.get(param, f"Param {param}")
            print(f"{name} set to {value}")
            self._push_radio_info()
        else:
            print("Failed to set parameter")

    def do_channel(self, arg):
        """
        Get or set the radio channel.

        Usage:
            channel         - Show current channel
            channel <n>     - Set channel (11-26 for 802.15.4)

        Example:
            channel
            channel 26
        """
        if arg:
            try:
                ch = int(arg)
                if self.radio.set_channel(ch):
                    print(f"Channel set to {ch}")
                    self._push_radio_info()
                else:
                    print("Failed to set channel")
            except ValueError:
                print("Invalid channel number")
        else:
            ch = self.radio.get_channel()
            if ch is not None:
                print(f"Channel: {ch}")
            else:
                print("Failed to get channel")

    def do_power(self, arg):
        """
        Get or set the TX power.

        Usage:
            power           - Show current TX power
            power <dBm>     - Set TX power

        Example:
            power
            power 5
        """
        if arg:
            try:
                pwr = int(arg)
                if self.radio.set_tx_power(pwr):
                    print(f"TX power set to {pwr} dBm")
                    self._push_radio_info()
                else:
                    print("Failed to set TX power")
            except ValueError:
                print("Invalid power value")
        else:
            pwr = self.radio.get_tx_power()
            if pwr is not None:
                print(f"TX Power: {pwr} dBm")
            else:
                print("Failed to get TX power")

    def do_rssi(self, arg):
        """Read current RSSI value."""
        rssi = self.radio.get_rssi()
        if rssi is not None:
            print(f"RSSI: {rssi} dBm")
        else:
            print("Failed to read RSSI")

    # -------------------------------------------------------------------------
    # Radio control commands
    # -------------------------------------------------------------------------

    def do_rx(self, arg):
        """
        Control radio receiver.

        Usage:
            rx on   - Turn on receiver
            rx off  - Turn off receiver
        """
        if arg.lower() == 'on':
            if self.radio.rx_on():
                print("Receiver ON")
            else:
                print("Failed to turn on receiver")
        elif arg.lower() == 'off':
            if self.radio.rx_off():
                print("Receiver OFF")
            else:
                print("Failed to turn off receiver")
        else:
            print("Usage: rx on|off")

    # -------------------------------------------------------------------------
    # Transmit commands
    # -------------------------------------------------------------------------

    def do_tx(self, arg):
        """
        Transmit a raw frame.

        Usage:
            tx <hex_data> [channel]

        Example:
            tx 0102030405060708
            tx DEADBEEF 26
        """
        parts = arg.split()
        if not parts:
            print("Usage: tx <hex_data> [channel]")
            return

        try:
            data = bytes.fromhex(parts[0])
        except ValueError:
            print("Invalid hex data")
            return

        channel = None
        if len(parts) > 1:
            try:
                channel = int(parts[1])
            except ValueError:
                print("Invalid channel number")
                return

        if self.radio.send_frame(data, channel):
            print(f"Transmitted {len(data)} bytes")
        else:
            print("Transmission failed")

    # -------------------------------------------------------------------------
    # Sniffing commands
    # -------------------------------------------------------------------------

    def do_sniff(self, arg):
        """
        Start or stop packet sniffing.

        Usage:
            sniff             - Toggle sniffing
            sniff on|start    - Start sniffing
            sniff off|stop    - Stop sniffing
        """
        action = arg.lower().strip()
        start = action in ('on', 'start')
        stop = action in ('off', 'stop')
        if start or (not action and not self._sniffing):
            self._sniffing = True
            # Enable promiscuous RX on the device so it emits RX_FRAME events.
            self.radio.rx_on()
            self.radio.set_rx_callback(self._rx_callback)
            print("Sniffing started. Press Enter to show prompt.")
        elif stop or (not action and self._sniffing):
            self._sniffing = False
            self.radio.set_rx_callback(None)
            self.radio.rx_off()
            print("Sniffing stopped.")
        else:
            print("Usage: sniff [on|off]")

    def _rx_callback(self, frame: RxFrame):
        """Handle received frame during sniffing."""
        hex_data = frame.data.hex()
        print(f"\n[RX] RSSI:{frame.rssi:4d} dBm  LQI:{frame.lqi:3d}  "
              f"Len:{len(frame.data):3d}  Data: {hex_data[:64]}{'...' if len(hex_data) > 64 else ''}")

        # Broadcast to web clients if webserver is running
        if self._webserver:
            self._webserver.broadcast_rx_frame(
                frame.data, frame.rssi, frame.lqi, frame.timestamp
            )

    # -------------------------------------------------------------------------
    # Scanning commands
    # -------------------------------------------------------------------------

    def do_scan(self, arg):
        """
        Perform RSSI channel scan.

        Usage:
            scan [start_ch] [end_ch] [dwell_ms]

        Default: scan channels 11-26 with 10ms dwell time

        Example:
            scan
            scan 11 26 50
            scan 15 20
        """
        parts = arg.split()

        start_ch = 11
        end_ch = 26
        dwell_ms = 10

        if len(parts) >= 1:
            start_ch = int(parts[0])
        if len(parts) >= 2:
            end_ch = int(parts[1])
        if len(parts) >= 3:
            dwell_ms = int(parts[2])

        print(f"Scanning channels {start_ch}-{end_ch} (dwell: {dwell_ms}ms)...")
        print("Press Enter to stop scan.\n")

        self._scan_results = []
        self._scanning = True

        self.radio.set_scan_callback(self._scan_callback)
        self.radio.start_scan(start_ch, end_ch, dwell_ms)

        # Wait for one full scan
        try:
            timeout = (end_ch - start_ch + 1) * dwell_ms / 1000.0 + 1.0
            time.sleep(timeout)
        except KeyboardInterrupt:
            pass

        self.radio.stop_scan()
        self._scanning = False
        self.radio.set_scan_callback(None)

        # Display results
        if self._scan_results:
            print("\nScan Results:")
            print("-" * 50)

            # Group by channel
            by_channel = {}
            for r in self._scan_results:
                by_channel[r.channel] = r.rssi

            for ch in sorted(by_channel.keys()):
                rssi = by_channel[ch]
                bar = '#' * max(0, (rssi + 100) // 2)
                print(f"  Ch {ch:2d}: {rssi:4d} dBm  |{bar}")

    def _scan_callback(self, result: ScanResult):
        """Handle scan result."""
        self._scan_results.append(result)
        if self._scanning:
            print(f"  Ch {result.channel:2d}: {result.rssi:4d} dBm")

    def do_scan_continuous(self, arg):
        """
        Start continuous scanning (background).

        Usage:
            scan_continuous [start_ch] [end_ch] [dwell_ms]

        Use 'scan_stop' to stop.
        """
        parts = arg.split()

        start_ch = 11
        end_ch = 26
        dwell_ms = 100

        if len(parts) >= 1:
            start_ch = int(parts[0])
        if len(parts) >= 2:
            end_ch = int(parts[1])
        if len(parts) >= 3:
            dwell_ms = int(parts[2])

        self._scanning = True
        self.radio.set_scan_callback(self._scan_callback)
        self.radio.start_scan(start_ch, end_ch, dwell_ms)
        print("Continuous scanning started. Use 'scan_stop' to stop.")

    def do_scan_stop(self, arg):
        """Stop continuous scanning."""
        self.radio.stop_scan()
        self._scanning = False
        self.radio.set_scan_callback(None)
        print("Scanning stopped.")

    # -------------------------------------------------------------------------
    # Fast scan commands
    # -------------------------------------------------------------------------

    def do_fastscan(self, arg):
        """
        Start/stop fast multi-channel RSSI scanning.

        Usage:
            fastscan start [start_ch] [end_ch]  - Start fast scanning
            fastscan stop                       - Stop fast scanning

        The fast scanner sweeps through all channels as quickly as possible
        and sends all RSSI values in a single message.

        Channel ranges depend on radio mode:
            2.4 GHz (IEEE 802.15.4):  channels 11-26
            Sub-GHz 863 MHz (Europe): channels 0-33
            Sub-GHz 915 MHz (US):     channels 0-128
            Sub-GHz 920 MHz (Japan):  channels 0-37

        Example:
            fastscan start 11 26   - Scan 2.4 GHz channels
            fastscan start 0 33    - Scan Sub-GHz 863 MHz channels
            fastscan start         - Auto-detect channel range
            fastscan stop          - Stop scanning
        """
        args = arg.split()
        if not args:
            print("Usage: fastscan start [start_ch] [end_ch] | fastscan stop")
            return

        cmd = args[0].lower()
        if cmd == 'start':
            # Try to auto-detect channel range from radio if not specified
            if len(args) > 2:
                start_ch = int(args[1])
                end_ch = int(args[2])
            elif len(args) > 1:
                start_ch = int(args[1])
                end_ch = self.radio.get_param(RadioParam.CHANNEL_MAX) or 26
            else:
                # Auto-detect from radio
                start_ch = self.radio.get_param(RadioParam.CHANNEL_MIN)
                end_ch = self.radio.get_param(RadioParam.CHANNEL_MAX)
                if start_ch is None or end_ch is None:
                    # Fallback to 2.4 GHz defaults
                    start_ch = 11
                    end_ch = 26

            self.radio.set_fast_scan_callback(self._fast_scan_callback)
            self.radio.start_fast_scan(start_ch, end_ch)
            print(f"Fast scanning channels {start_ch}-{end_ch}...")
            print("Use 'fastscan stop' to stop.")

        elif cmd == 'stop':
            self.radio.stop_fast_scan()
            self.radio.set_fast_scan_callback(None)
            print("Fast scanning stopped.")
        else:
            print("Usage: fastscan start [start_ch] [end_ch] | fastscan stop")

    def _fast_scan_callback(self, result: FastScanResult):
        """Handle fast scan result from device."""
        # Compact printout: just sequence number and RSSI values
        rssi_str = ' '.join(f"{r:4d}" for r in result.rssi_values)
        print(f"\r[{result.seq:5d}] {rssi_str}", end='', flush=True)

        # Broadcast to web clients if webserver is running
        if self._webserver:
            self._webserver.broadcast_spectrum(
                result.seq, result.start_ch, result.end_ch,
                result.rssi_values, result.timestamp
            )

    # -------------------------------------------------------------------------
    # Jamming commands
    # -------------------------------------------------------------------------

    def do_jam(self, arg):
        """
        Start/stop channel jamming by continuous transmission.

        Usage:
            jam start [channel] [interval_ms]  - Start jamming
            jam stop                           - Stop jamming

        Arguments:
            channel:     Channel to jam (default: current channel or 26)
            interval_ms: Interval between transmissions in ms (default: 5)

        The jammer sends packets continuously on the specified channel,
        interfering with any communication on that channel.

        WARNING: Jamming radio communications may be illegal in your
        jurisdiction. Use only for authorized testing purposes.

        Example:
            jam start 26 5   - Jam channel 26 with 5ms interval
            jam start 15     - Jam channel 15 with default interval
            jam stop         - Stop jamming
        """
        args = arg.split()
        if not args:
            print("Usage: jam start [channel] [interval_ms] | jam stop")
            return

        cmd = args[0].lower()
        if cmd == 'start':
            # Get channel
            if len(args) > 1:
                channel = int(args[1])
            else:
                channel = self.radio.get_param(RadioParam.CHANNEL) or 26

            # Get interval
            if len(args) > 2:
                interval_ms = int(args[2])
            else:
                interval_ms = 5

            self.radio.start_jam(channel, interval_ms)
            print(f"Jamming channel {channel} with {interval_ms}ms interval...")
            print("WARNING: This may be illegal. Use only for authorized testing.")
            print("Use 'jam stop' to stop.")

        elif cmd == 'stop':
            self.radio.stop_jam()
            print("Jamming stopped.")
        else:
            print("Usage: jam start [channel] [interval_ms] | jam stop")

    # -------------------------------------------------------------------------
    # Web server commands
    # -------------------------------------------------------------------------

    def do_webserver(self, arg):
        """
        Start/stop web-based spectrum visualization server.

        Usage:
            webserver start [http_port]  - Start web server
            webserver stop               - Stop web server
            webserver status             - Show server status

        Default port: HTTP=8080 (WebSocket is automatically HTTP+1)

        The web interface provides:
        - Real-time 2D spectrum bar chart
        - 3D waterfall display (drag to rotate)
        - Radio information panel
        - RSSI statistics

        Example:
            webserver start       - Start on default HTTP port 8080 (WS 8081)
            webserver start 8000  - Start on HTTP port 8000 (WS 8001)
            webserver stop        - Stop the server
        """
        if not WEBSERVER_AVAILABLE:
            print("Web server not available. Install dependencies:")
            print("  pip install websockets")
            return

        args = arg.split()
        if not args:
            print("Usage: webserver start [http_port] | stop | status")
            return

        cmd = args[0].lower()

        if cmd == 'start':
            if self._webserver:
                print("Web server already running")
                return

            http_port = 8080

            if len(args) > 1:
                http_port = int(args[1])

            try:
                self._webserver = SerialRadioWebServer(http_port)
                self._webserver.set_command_handler(self._handle_web_command)
                self._webserver.start()

                # Enable debug callback to stream to web clients
                self.radio.set_debug_callback(self._debug_callback)

                # Send initial radio info
                info = self.radio.get_radio_info()
                if info:
                    self._webserver.broadcast_radio_info(info)

                print(f"\nOpen http://localhost:{http_port}/ in your browser")
                print("Use 'fastscan start' to begin streaming spectrum data")
                print("Debug output is now streaming to web clients")

            except Exception as e:
                print(f"Failed to start web server: {e}")
                self._webserver = None

        elif cmd == 'stop':
            if self._webserver:
                self._webserver.stop()
                self._webserver = None
                print("Web server stopped")
            else:
                print("Web server not running")

        elif cmd == 'status':
            if self._webserver:
                print("Web server: RUNNING")
                print(f"  HTTP:      http://localhost:{self._webserver.http_port}/")
                print(f"  WebSocket: ws://localhost:{self._webserver.ws_port}/")
            else:
                print("Web server: STOPPED")

        else:
            print("Usage: webserver start [http_port] | stop | status")

    def _handle_web_command(self, cmd: str, params: dict) -> any:
        """Handle commands from web UI - all commands go through CLI."""
        if cmd == 'cli_command':
            # Execute arbitrary CLI command and capture output
            cli_text = params.get('text', '').strip()
            if not cli_text:
                return {'error': 'missing text parameter'}

            import io
            import sys

            # Need to capture both self.stdout (used by help) and sys.stdout (used by print)
            output = io.StringIO()
            old_self_stdout = self.stdout
            old_sys_stdout = sys.stdout
            try:
                self.stdout = output
                sys.stdout = output
                # Force flush before command
                self.onecmd(cli_text)
                # Get output and flush
                output.flush()
                result = output.getvalue()
                # Debug: print to original stdout
                old_sys_stdout.write(f"[WEB CMD] '{cli_text}' -> {len(result)} chars\n")
                old_sys_stdout.flush()
                return {'output': result if result else '(command executed)', 'command': cli_text}
            except Exception as e:
                return {'error': str(e), 'command': cli_text}
            finally:
                self.stdout = old_self_stdout
                sys.stdout = old_sys_stdout

        else:
            return {'error': f'unknown command: {cmd}'}

    # -------------------------------------------------------------------------
    # Debug commands
    # -------------------------------------------------------------------------

    def do_debug(self, arg):
        """
        Toggle debug output display.

        Usage:
            debug on   - Show debug output from device
            debug off  - Hide debug output
        """
        if arg.lower() == 'on':
            self.radio.set_debug_callback(self._debug_callback)
            print("Debug output enabled")
        elif arg.lower() == 'off':
            self.radio.set_debug_callback(None)
            print("Debug output disabled")
        else:
            print("Usage: debug on|off")

    def _debug_callback(self, text: str):
        """Handle debug text from device."""
        import time
        timestamp = time.time()
        for line in text.strip().split('\n'):
            if line:
                print(f"[DBG] {line}")
        # Broadcast to web clients if webserver is running
        if self._webserver and text.strip():
            self._webserver.broadcast_debug(text, timestamp)

    # -------------------------------------------------------------------------
    # Heartbeat commands
    # -------------------------------------------------------------------------

    def do_heartbeat(self, arg):
        """
        Toggle heartbeat display.

        Usage:
            heartbeat on   - Show heartbeats from device
            heartbeat off  - Hide heartbeats
        """
        if arg.lower() == 'on':
            self.radio.set_heartbeat_callback(self._heartbeat_callback)
            print("Heartbeat display enabled")
        elif arg.lower() == 'off':
            self.radio.set_heartbeat_callback(None)
            print("Heartbeat display disabled")
        else:
            print("Usage: heartbeat on|off")

    def _heartbeat_callback(self, hb: Heartbeat):
        """Handle heartbeat from device."""
        print(f"\n[HB] seq={hb.seq} uptime={hb.uptime}s")

    # -------------------------------------------------------------------------
    # Utility commands
    # -------------------------------------------------------------------------

    def do_ports(self, arg):
        """List available serial ports."""
        ports = list_serial_ports()
        if ports:
            print("Available serial ports:")
            for p in ports:
                print(f"  {p}")
        else:
            print("No serial ports found")

    def do_quit(self, arg):
        """Exit the CLI."""
        print("Goodbye!")
        return True

    def do_exit(self, arg):
        """Exit the CLI."""
        return self.do_quit(arg)

    def do_EOF(self, arg):
        """Handle Ctrl-D."""
        print()
        return self.do_quit(arg)

    # -------------------------------------------------------------------------
    # Helpers
    # -------------------------------------------------------------------------

    def _push_radio_info(self, info=None):
        """Broadcast the current radio parameters to any connected web clients
        so the web UI's info panel (channel, PAN ID, TX power, ...) reflects the
        latest state. No-op when the web server is not running. Pass an already
        fetched info dict to avoid re-querying the radio."""
        if self._webserver is None:
            return
        if info is None:
            info = self.radio.get_radio_info()
        if info:
            self._webserver.broadcast_radio_info(info)

    def _parse_param(self, arg: str) -> Optional[int]:
        """Parse parameter name or number."""
        param_map = {
            'channel': RadioParam.CHANNEL,
            'ch': RadioParam.CHANNEL,
            'power': RadioParam.TXPOWER,
            'txpower': RadioParam.TXPOWER,
            'rssi': RadioParam.RSSI,
            'pan': RadioParam.PAN_ID,
            'panid': RadioParam.PAN_ID,
            'addr': RadioParam.SHORT_ADDR,
            'address': RadioParam.SHORT_ADDR,
            'lqi': RadioParam.LAST_LINK_QUALITY,
        }

        arg_lower = arg.lower()
        if arg_lower in param_map:
            return param_map[arg_lower]

        try:
            return int(arg, 0)
        except ValueError:
            return None

    def _parse_value(self, arg: str) -> int:
        """Parse value (supports hex with 0x prefix)."""
        return int(arg, 0)

    def emptyline(self):
        """Don't repeat last command on empty line."""
        pass


def main():
    parser = argparse.ArgumentParser(
        description='Serial Radio Control CLI',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    %(prog)s /dev/ttyUSB0
    %(prog)s /dev/tty.usbserial-1234 --baud 115200
    %(prog)s --list

Interactive Commands:
    ping            - Test connectivity
    info            - Show radio information
    get <param>     - Get parameter (channel, power, rssi, etc.)
    set <p> <v>     - Set parameter value
    channel [n]     - Get/set channel
    power [dBm]     - Get/set TX power
    rssi            - Read current RSSI
    tx <hex> [ch]   - Transmit raw frame
    sniff           - Toggle packet sniffing
    scan [s] [e]    - RSSI channel scan
    quit            - Exit
"""
    )

    parser.add_argument('port', nargs='?', help='Serial port')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-t', '--timeout', type=float, default=2.0,
                        help='Response timeout (default: 2.0)')
    parser.add_argument('-l', '--list', action='store_true',
                        help='List available serial ports')

    args = parser.parse_args()

    if args.list:
        ports = list_serial_ports()
        if ports:
            print("Available serial ports:")
            for p in ports:
                print(f"  {p}")
        else:
            print("No serial ports found")
        sys.exit(0)

    if not args.port:
        parser.print_help()
        sys.exit(1)

    # Create radio connection
    radio = SerialRadio(args.port, args.baud, args.timeout)

    try:
        print(f"Connecting to {args.port}...")
        if radio.connect():
            print(f"Connected! Device version: {radio.version}")
        else:
            print("Warning: Device not responding to PING")

        # Start CLI
        cli = SerialRadioCLI(radio)

        # Enable debug output by default
        cli.do_debug('on')

        cli.cmdloop()

    except SerialRadioError as e:
        print(f"Error: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nInterrupted")
    finally:
        radio.disconnect()


if __name__ == '__main__':
    main()

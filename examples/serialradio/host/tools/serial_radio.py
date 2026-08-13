"""
Serial Radio Controller Library.

High-level Python API for controlling a Contiki-NG serial radio node.
"""

import threading
import time
from typing import Optional, Callable, Dict, Any, List
from dataclasses import dataclass
from queue import Queue, Empty

import serial
import cbor2

from .slip import SlipDecoder, slip_encode
from .crc16 import crc16_verify, crc16_append
from .protocol import Command, Event, ErrorCode, RadioParam, Key, get_error_message


@dataclass
class RxFrame:
    """Received radio frame."""
    data: bytes
    rssi: int
    lqi: int
    timestamp: float


@dataclass
class ScanResult:
    """RSSI scan result for a channel."""
    channel: int
    rssi: int
    timestamp: float


@dataclass
class Heartbeat:
    """Heartbeat from device."""
    seq: int
    uptime: int
    timestamp: float


@dataclass
class FastScanResult:
    """Fast scan result with RSSI for all channels."""
    seq: int
    start_ch: int
    end_ch: int
    rssi_values: list  # List of RSSI values for each channel
    timestamp: float


class SerialRadioError(Exception):
    """Serial radio communication error."""
    pass


class SerialRadio:
    """
    Serial Radio Controller.

    Provides a high-level API for communicating with a Contiki-NG
    serial radio node over a serial port.

    Example:
        radio = SerialRadio('/dev/ttyUSB0')
        radio.connect()

        # Get current channel
        channel = radio.get_param(RadioParam.CHANNEL)
        print(f"Channel: {channel}")

        # Set channel
        radio.set_param(RadioParam.CHANNEL, 26)

        # Listen for frames
        radio.set_rx_callback(lambda frame: print(f"RX: {frame}"))

        radio.disconnect()
    """

    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 2.0):
        """
        Initialize serial radio controller.

        Args:
            port: Serial port path (e.g., '/dev/ttyUSB0')
            baudrate: Serial baud rate (default 115200)
            timeout: Response timeout in seconds
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout

        self._serial: Optional[serial.Serial] = None
        self._decoder = SlipDecoder()
        self._msg_id: int = 0
        self._pending: Dict[int, Queue] = {}
        self._running: bool = False
        self._rx_thread: Optional[threading.Thread] = None
        self._lock = threading.Lock()

        # Callbacks
        self._rx_callback: Optional[Callable[[RxFrame], None]] = None
        self._scan_callback: Optional[Callable[[ScanResult], None]] = None
        self._heartbeat_callback: Optional[Callable[[Heartbeat], None]] = None
        self._fast_scan_callback: Optional[Callable[[FastScanResult], None]] = None
        self._debug_callback: Optional[Callable[[str], None]] = None

        # Version info from PONG
        self.version: Optional[str] = None

    def connect(self) -> bool:
        """
        Connect to the serial radio.

        Returns:
            True if connected and verified, False otherwise
        """
        try:
            self._serial = serial.Serial(
                self.port,
                self.baudrate,
                timeout=0.1  # Non-blocking read
            )

            # Start receive thread
            self._running = True
            self._rx_thread = threading.Thread(target=self._rx_loop, daemon=True)
            self._rx_thread.start()

            # Verify connection with PING
            time.sleep(0.5)  # Wait for device to be ready
            return self.ping()

        except serial.SerialException as e:
            self._running = False
            raise SerialRadioError(f"Failed to open serial port: {e}")

    def disconnect(self):
        """Disconnect from the serial radio."""
        self._running = False
        if self._rx_thread:
            self._rx_thread.join(timeout=1.0)
        if self._serial:
            self._serial.close()
            self._serial = None

    def is_connected(self) -> bool:
        """Check if connected."""
        return self._serial is not None and self._serial.is_open

    def ping(self) -> bool:
        """
        Send PING and wait for PONG.

        Returns:
            True if PONG received, False otherwise
        """
        try:
            response = self._send_command({
                Key.TYPE: Command.PING
            })

            if response and response.get(Key.TYPE) == Event.PONG:
                self.version = response.get(Key.VERSION)
                return True
            return False

        except SerialRadioError:
            return False

    def get_param(self, param: int) -> Optional[int]:
        """
        Get a radio parameter value.

        Args:
            param: RadioParam constant

        Returns:
            Parameter value or None on error
        """
        response = self._send_command({
            Key.TYPE: Command.GET_PARAM,
            Key.PARAM: param
        })

        if response and response.get(Key.TYPE) == Event.PARAM_RESPONSE:
            return response.get(Key.VALUE)
        return None

    def set_param(self, param: int, value: int) -> bool:
        """
        Set a radio parameter value.

        Args:
            param: RadioParam constant
            value: New value

        Returns:
            True if successful
        """
        response = self._send_command({
            Key.TYPE: Command.SET_PARAM,
            Key.PARAM: param,
            Key.VALUE: value
        })

        if response and response.get(Key.TYPE) == Event.PARAM_RESPONSE:
            return response.get(Key.VALUE) == value
        return False

    def get_channel(self) -> Optional[int]:
        """Get current radio channel."""
        return self.get_param(RadioParam.CHANNEL)

    def set_channel(self, channel: int) -> bool:
        """Set radio channel."""
        return self.set_param(RadioParam.CHANNEL, channel)

    def get_tx_power(self) -> Optional[int]:
        """Get TX power in dBm."""
        return self.get_param(RadioParam.TXPOWER)

    def set_tx_power(self, power: int) -> bool:
        """Set TX power in dBm."""
        return self.set_param(RadioParam.TXPOWER, power)

    def get_rssi(self) -> Optional[int]:
        """Get current RSSI reading."""
        return self.get_param(RadioParam.RSSI)

    def rx_on(self) -> bool:
        """Turn on radio receiver."""
        response = self._send_command({
            Key.TYPE: Command.RX_ON
        })
        return response is not None and response.get(Key.TYPE) == Event.PARAM_RESPONSE

    def rx_off(self) -> bool:
        """Turn off radio receiver."""
        response = self._send_command({
            Key.TYPE: Command.RX_OFF
        })
        return response is not None and response.get(Key.TYPE) == Event.PARAM_RESPONSE

    def send_frame(self, data: bytes, channel: Optional[int] = None) -> bool:
        """
        Send a raw radio frame.

        Args:
            data: Frame data
            channel: Optional channel to send on (uses current if None)

        Returns:
            True if transmission successful
        """
        cmd = {
            Key.TYPE: Command.TX_RAW_FRAME,
            Key.FRAME: data
        }
        if channel is not None:
            cmd[Key.CHANNEL] = channel

        response = self._send_command(cmd)

        if response and response.get(Key.TYPE) == Event.TX_RESPONSE:
            return response.get(Key.VALUE) == 0
        return False

    def start_scan(self, start_ch: int = 11, end_ch: int = 26,
                   dwell_ms: int = 10) -> bool:
        """
        Start RSSI channel scanning.

        Args:
            start_ch: First channel to scan
            end_ch: Last channel to scan
            dwell_ms: Dwell time per channel in milliseconds

        Returns:
            True if scan started successfully
        """
        response = self._send_command({
            Key.TYPE: Command.RSSI_SCAN_START,
            Key.START_CH: start_ch,
            Key.END_CH: end_ch,
            Key.DWELL: dwell_ms
        }, expect_response=False)

        # Scan start doesn't have a specific response
        return True

    def stop_scan(self) -> bool:
        """Stop RSSI channel scanning."""
        self._send_command({
            Key.TYPE: Command.RSSI_SCAN_STOP
        }, expect_response=False)
        return True

    def start_fast_scan(self, start_ch: int = 11, end_ch: int = 26) -> bool:
        """
        Start fast RSSI channel scanning.

        Scans all channels rapidly and returns all RSSI values in a single message.
        Results are delivered via the fast_scan callback.

        Args:
            start_ch: First channel to scan (default 11)
            end_ch: Last channel to scan (default 26)

        Returns:
            True if scan started successfully
        """
        self._send_command({
            Key.TYPE: Command.FAST_SCAN_START,
            Key.START_CH: start_ch,
            Key.END_CH: end_ch
        }, expect_response=False)
        return True

    def stop_fast_scan(self) -> bool:
        """Stop fast RSSI channel scanning."""
        self._send_command({
            Key.TYPE: Command.FAST_SCAN_STOP
        }, expect_response=False)
        return True

    def start_jam(self, channel: int = 26, interval_ms: int = 5,
                  payload: Optional[bytes] = None) -> bool:
        """
        Start jamming on a channel by continuously transmitting.

        Args:
            channel: Channel to jam (default 26)
            interval_ms: Interval between transmissions in ms (default 5)
            payload: Optional custom payload bytes (default: 100 bytes of 0xAA)

        Returns:
            True if command sent successfully
        """
        cmd = {
            Key.TYPE: Command.JAM_START,
            Key.CHANNEL: channel,
            Key.DWELL: interval_ms,
        }
        if payload is not None:
            cmd[Key.FRAME] = payload
        self._send_command(cmd, expect_response=False)
        return True

    def stop_jam(self) -> bool:
        """Stop jamming."""
        self._send_command({
            Key.TYPE: Command.JAM_STOP
        }, expect_response=False)
        return True

    def set_rx_callback(self, callback: Optional[Callable[[RxFrame], None]]):
        """Set callback for received frames."""
        self._rx_callback = callback

    def set_scan_callback(self, callback: Optional[Callable[[ScanResult], None]]):
        """Set callback for scan results."""
        self._scan_callback = callback

    def set_heartbeat_callback(self, callback: Optional[Callable[[Heartbeat], None]]):
        """Set callback for heartbeat events."""
        self._heartbeat_callback = callback

    def set_fast_scan_callback(self, callback: Optional[Callable[[FastScanResult], None]]):
        """Set callback for fast scan results."""
        self._fast_scan_callback = callback

    def set_debug_callback(self, callback: Optional[Callable[[str], None]]):
        """Set callback for debug text output."""
        self._debug_callback = callback

    def get_radio_info(self) -> Dict[str, Any]:
        """
        Get comprehensive radio information.

        Returns:
            Dictionary with radio parameters
        """
        info = {}

        params = [
            (RadioParam.CHANNEL, 'channel'),
            (RadioParam.TXPOWER, 'tx_power'),
            (RadioParam.PAN_ID, 'pan_id'),
            (RadioParam.SHORT_ADDR, 'short_addr'),
            (RadioParam.RX_MODE, 'rx_mode'),
            (RadioParam.TX_MODE, 'tx_mode'),
            (RadioParam.CHANNEL_MIN, 'channel_min'),
            (RadioParam.CHANNEL_MAX, 'channel_max'),
            (RadioParam.TXPOWER_MIN, 'tx_power_min'),
            (RadioParam.TXPOWER_MAX, 'tx_power_max'),
        ]

        for param, name in params:
            value = self.get_param(param)
            if value is not None:
                info[name] = value

        return info

    # -------------------------------------------------------------------------
    # Internal methods
    # -------------------------------------------------------------------------

    def _next_msg_id(self) -> int:
        """Get next message ID."""
        with self._lock:
            self._msg_id = (self._msg_id + 1) % 256
            return self._msg_id

    def _send_command(self, cmd: Dict[str, Any],
                      expect_response: bool = True) -> Optional[Dict[str, Any]]:
        """
        Send a command and optionally wait for response.

        Args:
            cmd: Command dictionary
            expect_response: Whether to wait for response

        Returns:
            Response dictionary or None
        """
        if not self.is_connected():
            raise SerialRadioError("Not connected")

        msg_id = self._next_msg_id()
        cmd[Key.ID] = msg_id

        # Create response queue
        response_queue: Queue = Queue()
        if expect_response:
            with self._lock:
                self._pending[msg_id] = response_queue

        try:
            # Encode and send
            cbor_data = cbor2.dumps(cmd)
            framed_data = crc16_append(cbor_data)
            slip_data = slip_encode(framed_data)

            self._serial.write(slip_data)
            self._serial.flush()

            if not expect_response:
                return None

            # Wait for response
            try:
                response = response_queue.get(timeout=self.timeout)
                return response
            except Empty:
                raise SerialRadioError("Response timeout")

        finally:
            # Clean up pending queue
            if expect_response:
                with self._lock:
                    self._pending.pop(msg_id, None)

    def _rx_loop(self):
        """Receive loop running in background thread."""
        while self._running:
            try:
                if self._serial and self._serial.in_waiting:
                    data = self._serial.read(self._serial.in_waiting)
                    frames, debug_text = self._decoder.feed(data)

                    # Handle debug text
                    if debug_text and self._debug_callback:
                        self._debug_callback(debug_text)

                    # Handle frames
                    for frame in frames:
                        self._handle_frame(frame)
                else:
                    time.sleep(0.01)

            except Exception as e:
                if self._running:
                    print(f"RX error: {e}")
                break

    def _handle_frame(self, frame: bytes):
        """Handle a received SLIP frame."""
        # Verify CRC
        if not crc16_verify(frame):
            print(f"CRC verification failed: {frame.hex()} (len={len(frame)})")
            return

        # Decode CBOR (strip CRC)
        try:
            msg = cbor2.loads(frame[:-2])
        except Exception as e:
            print(f"CBOR decode error: {e}")
            return

        msg_type = msg.get(Key.TYPE)
        msg_id = msg.get(Key.ID, 0)

        # Check for pending response
        with self._lock:
            if msg_id in self._pending:
                self._pending[msg_id].put(msg)
                return

        # Handle events
        if msg_type == Event.RX_FRAME:
            if self._rx_callback:
                rx_frame = RxFrame(
                    data=msg.get(Key.FRAME, b''),
                    rssi=msg.get(Key.RSSI, 0),
                    lqi=msg.get(Key.LQI, 0),
                    timestamp=time.time()
                )
                self._rx_callback(rx_frame)

        elif msg_type == Event.RSSI_SCAN_RESULT:
            if self._scan_callback:
                result = ScanResult(
                    channel=msg.get(Key.CHANNEL, 0),
                    rssi=msg.get(Key.RSSI, 0),
                    timestamp=time.time()
                )
                self._scan_callback(result)

        elif msg_type == Event.HEARTBEAT:
            if self._heartbeat_callback:
                hb = Heartbeat(
                    seq=msg.get('s', 0),
                    uptime=msg.get('u', 0),
                    timestamp=time.time()
                )
                self._heartbeat_callback(hb)

        elif msg_type == Event.FAST_SCAN_RESULT:
            if self._fast_scan_callback:
                result = FastScanResult(
                    seq=msg.get(Key.SEQ, 0),
                    start_ch=msg.get(Key.START_CH, 0),
                    end_ch=msg.get(Key.END_CH, 0),
                    rssi_values=msg.get(Key.RSSI_ARRAY, []),
                    timestamp=time.time()
                )
                self._fast_scan_callback(result)

        elif msg_type == Event.ERROR:
            error_code = msg.get(Key.ERROR, 0)
            error_msg = f"Error from device: {get_error_message(error_code)}"
            print(error_msg)
            # Also send through debug callback so it appears in web console
            if self._debug_callback:
                self._debug_callback(f"[ERROR] {error_msg}\n")


def list_serial_ports() -> List[str]:
    """
    List available serial ports.

    Returns:
        List of port names
    """
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()
    return [p.device for p in ports]

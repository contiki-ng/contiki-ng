"""
Serial Radio Web Interface Server.

Provides a web-based interface for controlling and monitoring a
serial radio node, including spectrum visualization, packet display,
and radio configuration.
"""

import asyncio
import json
import threading
import time
from http.server import HTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
from typing import Optional, Set, Callable
import os

# Try to import websockets, provide helpful error if not available
try:
    import websockets
    from websockets.server import serve as websocket_serve
    WEBSOCKETS_AVAILABLE = True
except ImportError:
    WEBSOCKETS_AVAILABLE = False


class SerialRadioWebServer:
    """
    Web server for serial radio control and monitoring.

    Serves a web interface and streams data via WebSocket including:
    - Spectrum/RSSI visualization
    - Received packets display
    - Radio configuration and control
    """

    def __init__(self, http_port: int = 8080):
        """
        Initialize the serial radio web server.

        Args:
            http_port: Port for HTTP server (default 8080)
                       WebSocket port is automatically http_port + 1
        """
        if not WEBSOCKETS_AVAILABLE:
            raise ImportError(
                "websockets package required. Install with: pip install websockets"
            )

        # Command handler callback - set by CLI to handle web commands
        self._command_handler: Optional[Callable[[str, dict], None]] = None

        self.http_port = http_port
        self.ws_port = http_port + 1  # WebSocket is always HTTP port + 1

        self._running = False
        self._http_thread: Optional[threading.Thread] = None
        self._ws_thread: Optional[threading.Thread] = None
        self._clients: Set = set()
        self._loop: Optional[asyncio.AbstractEventLoop] = None

        # Data buffer for spectrum history (for 3D waterfall display)
        self._spectrum_history = []
        # Last radio info broadcast, replayed to each client on connect so a
        # freshly loaded page shows the current parameters without a refresh.
        self._last_radio_info = None
        self._max_history = 100  # Keep last 100 scans

        # Get the www directory path
        self._www_dir = Path(__file__).parent / "www"

    def start(self):
        """Start both HTTP and WebSocket servers."""
        if self._running:
            return

        self._running = True

        # Create www directory if it doesn't exist
        self._www_dir.mkdir(exist_ok=True)

        # Start HTTP server in background thread
        self._http_thread = threading.Thread(
            target=self._run_http_server,
            daemon=True
        )
        self._http_thread.start()

        # Start WebSocket server in background thread
        self._ws_thread = threading.Thread(
            target=self._run_ws_server,
            daemon=True
        )
        self._ws_thread.start()

        print(f"Serial Radio web server started:")
        print(f"  HTTP:      http://localhost:{self.http_port}/")
        print(f"  WebSocket: ws://localhost:{self.ws_port}/")

    def stop(self):
        """Stop the servers."""
        self._running = False

        # Close WebSocket connections
        if self._loop:
            asyncio.run_coroutine_threadsafe(self._close_clients(), self._loop)

    async def _close_clients(self):
        """Close all WebSocket clients."""
        for client in self._clients.copy():
            await client.close()
        self._clients.clear()

    def _run_http_server(self):
        """Run HTTP server in background."""
        www_dir = str(self._www_dir)

        # Custom handler that serves from www directory without changing cwd
        class Handler(SimpleHTTPRequestHandler):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, directory=www_dir, **kwargs)

            def log_message(self, format, *args):
                # Suppress HTTP access logs
                pass

        Handler.extensions_map.update({
            '.js': 'application/javascript',
            '.css': 'text/css',
        })

        try:
            server = HTTPServer(('', self.http_port), Handler)
            # Time out handle_request() so the loop can observe _running going
            # False and exit promptly (instead of blocking until the next
            # request), then release the listening socket.
            server.timeout = 0.5
            print(f"HTTP server listening on port {self.http_port}")
            while self._running:
                server.handle_request()
            server.server_close()
        except Exception as e:
            if self._running:
                print(f"HTTP server error: {e}")
                import traceback
                traceback.print_exc()

    def _run_ws_server(self):
        """Run WebSocket server in background."""
        self._loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self._loop)

        try:
            self._loop.run_until_complete(self._ws_main())
        except Exception as e:
            if self._running:
                print(f"WebSocket server error: {e}")
                import traceback
                traceback.print_exc()

    async def _ws_main(self):
        """Main WebSocket server coroutine."""
        try:
            # Bind to all interfaces so it works from browser
            async with websocket_serve(self._ws_handler, "0.0.0.0", self.ws_port):
                print(f"WebSocket server listening on port {self.ws_port}")
                while self._running:
                    await asyncio.sleep(0.1)
        except Exception as e:
            print(f"WebSocket serve error: {e}")
            import traceback
            traceback.print_exc()

    async def _ws_handler(self, websocket):
        """Handle WebSocket client connection."""
        self._clients.add(websocket)
        try:
            # Send current history on connect
            if self._spectrum_history:
                await websocket.send(json.dumps({
                    'type': 'history',
                    'data': self._spectrum_history[-50:]  # Last 50 scans
                }))

            # Bring the newly connected client up to date with the current
            # radio parameters so its info panel is populated immediately.
            if self._last_radio_info:
                await websocket.send(json.dumps({
                    'type': 'radio_info',
                    'info': self._last_radio_info
                }))

            # Keep connection alive and handle incoming messages
            async for message in websocket:
                try:
                    data = json.loads(message)
                    await self._handle_client_message(websocket, data)
                except json.JSONDecodeError:
                    pass
        except websockets.exceptions.ConnectionClosed:
            pass
        finally:
            self._clients.discard(websocket)

    async def _handle_client_message(self, websocket, data: dict):
        """Handle incoming message from client."""
        msg_type = data.get('type')

        if msg_type == 'get_history':
            await websocket.send(json.dumps({
                'type': 'history',
                'data': self._spectrum_history[-50:]
            }))

        elif msg_type == 'command':
            # Handle control commands from web UI
            cmd = data.get('cmd')
            params = data.get('params', {})
            if self._command_handler and cmd:
                try:
                    result = self._command_handler(cmd, params)
                    await websocket.send(json.dumps({
                        'type': 'command_result',
                        'cmd': cmd,
                        'success': True,
                        'result': result
                    }))
                except Exception as e:
                    await websocket.send(json.dumps({
                        'type': 'command_result',
                        'cmd': cmd,
                        'success': False,
                        'error': str(e)
                    }))

    def set_command_handler(self, handler: Optional[Callable[[str, dict], any]]):
        """Set callback for handling commands from web UI."""
        self._command_handler = handler

    def broadcast_spectrum(self, seq: int, start_ch: int, end_ch: int,
                          rssi_values: list, timestamp: float):
        """
        Broadcast spectrum data to all connected clients.

        Args:
            seq: Sequence number
            start_ch: Start channel
            end_ch: End channel
            rssi_values: List of RSSI values
            timestamp: Timestamp of measurement
        """
        data = {
            'type': 'spectrum',
            'seq': seq,
            'start_ch': start_ch,
            'end_ch': end_ch,
            'rssi': rssi_values,
            'timestamp': timestamp
        }

        # Add to history
        self._spectrum_history.append(data)
        if len(self._spectrum_history) > self._max_history:
            self._spectrum_history.pop(0)

        # Broadcast to all clients
        if self._loop and self._clients:
            asyncio.run_coroutine_threadsafe(
                self._broadcast(json.dumps(data)),
                self._loop
            )

    async def _broadcast(self, message: str):
        """Broadcast message to all connected clients."""
        if not self._clients:
            return

        # Send to all clients, removing any that fail
        disconnected = set()
        for client in self._clients:
            try:
                await client.send(message)
            except:
                disconnected.add(client)

        self._clients -= disconnected

    def broadcast_radio_info(self, info: dict):
        """Broadcast radio information to all clients."""
        # Cache unconditionally (even with no clients connected yet, e.g. the
        # initial broadcast at server start) so new clients can be brought up
        # to date on connect.
        self._last_radio_info = info

        data = {
            'type': 'radio_info',
            'info': info
        }

        if self._loop and self._clients:
            asyncio.run_coroutine_threadsafe(
                self._broadcast(json.dumps(data)),
                self._loop
            )

    def broadcast_rx_frame(self, frame_data: bytes, rssi: int, lqi: int, timestamp: float):
        """
        Broadcast received frame to all connected clients.

        Args:
            frame_data: Raw frame bytes
            rssi: RSSI value in dBm
            lqi: Link Quality Indicator
            timestamp: Timestamp of reception
        """
        data = {
            'type': 'rx_frame',
            'data': frame_data.hex(),
            'rssi': rssi,
            'lqi': lqi,
            'length': len(frame_data),
            'timestamp': timestamp
        }

        if self._loop and self._clients:
            asyncio.run_coroutine_threadsafe(
                self._broadcast(json.dumps(data)),
                self._loop
            )

    def broadcast_heartbeat(self, seq: int, uptime: int, timestamp: float):
        """Broadcast heartbeat event to all clients."""
        data = {
            'type': 'heartbeat',
            'seq': seq,
            'uptime': uptime,
            'timestamp': timestamp
        }

        if self._loop and self._clients:
            asyncio.run_coroutine_threadsafe(
                self._broadcast(json.dumps(data)),
                self._loop
            )

    def broadcast_debug(self, text: str, timestamp: float):
        """Broadcast debug text to all clients."""
        data = {
            'type': 'debug',
            'text': text,
            'timestamp': timestamp
        }

        if self._loop and self._clients:
            asyncio.run_coroutine_threadsafe(
                self._broadcast(json.dumps(data)),
                self._loop
            )


# Keep old name as alias for backwards compatibility
SpectrumWebServer = SerialRadioWebServer


def check_dependencies():
    """Check if required dependencies are installed."""
    missing = []

    if not WEBSOCKETS_AVAILABLE:
        missing.append('websockets')

    if missing:
        print("Missing dependencies for web server:")
        print(f"  pip install {' '.join(missing)}")
        return False

    return True

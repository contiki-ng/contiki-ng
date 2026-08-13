/**
 * Serial Radio Spectrum Analyzer
 *
 * Real-time spectrum visualization with 2D and 3D waterfall displays.
 */

class SpectrumAnalyzer {
    constructor() {
        this.canvas = document.getElementById('spectrumCanvas');
        this.ctx = this.canvas.getContext('2d');

        // WebSocket
        this.ws = null;
        this.wsConnected = false;

        // Data
        this.spectrumHistory = [];
        this.maxHistory = 50;
        this.currentSpectrum = null;

        // Display settings
        this.rssiMin = -120;
        this.rssiMax = -20;
        this.viewMode = '2d';  // '2d' or '3d'

        // Stats
        this.sampleCount = 0;
        this.lastUpdateTime = Date.now();
        this.scanRate = 0;
        this.rateCounter = 0;

        // Three.js 3D
        this.threeInitialized = false;
        this.threeScene = null;
        this.threeCamera = null;
        this.threeRenderer = null;
        this.threeMesh = null;

        this.init();
    }

    init() {
        // Setup canvas resize with ResizeObserver for better responsiveness
        this.resizeCanvas();
        window.addEventListener('resize', () => this.resizeCanvas());

        // Use ResizeObserver for container-based resizing
        if (typeof ResizeObserver !== 'undefined') {
            const container = this.canvas.parentElement;
            this.resizeObserver = new ResizeObserver(() => this.resizeCanvas());
            this.resizeObserver.observe(container);
        }

        // Setup view toggle
        document.getElementById('view2d').addEventListener('click', () => this.setViewMode('2d'));
        document.getElementById('view3d').addEventListener('click', () => this.setViewMode('3d'));

        // Three.js render loop
        const animate3D = () => {
            requestAnimationFrame(animate3D);
            if (this.viewMode === '3d' && this.threeInitialized) {
                this.threeRenderer.render(this.threeScene, this.threeCamera);
            }
        };
        animate3D();

        // Update surface data periodically
        setInterval(() => {
            if (this.viewMode === '3d') {
                this.updateThree3D();
            }
        }, 100);

        // Setup settings inputs
        document.getElementById('rssiMin').addEventListener('change', (e) => {
            this.rssiMin = parseInt(e.target.value);
        });
        document.getElementById('rssiMax').addEventListener('change', (e) => {
            this.rssiMax = parseInt(e.target.value);
        });
        document.getElementById('historyLen').addEventListener('change', (e) => {
            this.maxHistory = parseInt(e.target.value);
            while (this.spectrumHistory.length > this.maxHistory) {
                this.spectrumHistory.shift();
            }
        });

        // Setup control buttons - all send CLI commands
        document.getElementById('btnScanStart').addEventListener('click', () => {
            const startCh = document.getElementById('scanStartCh').value;
            const endCh = document.getElementById('scanEndCh').value;
            this.sendCliText(`fastscan start ${startCh} ${endCh}`);
        });
        document.getElementById('btnScanStop').addEventListener('click', () => {
            this.sendCliText('fastscan stop');
        });
        document.getElementById('btnSniffStart').addEventListener('click', () => {
            this.sendCliText('sniff');
        });
        document.getElementById('btnSniffStop').addEventListener('click', () => {
            this.sendCliText('sniff stop');
        });
        document.getElementById('btnSetChannel').addEventListener('click', () => {
            const ch = document.getElementById('channelInput').value;
            this.sendCliText(`channel ${ch}`);
        });
        document.getElementById('btnSetPower').addEventListener('click', () => {
            const pwr = document.getElementById('powerInput').value;
            this.sendCliText(`power ${pwr}`);
        });
        document.getElementById('btnSetPanId').addEventListener('click', () => {
            const pan = document.getElementById('panIdInput').value;
            this.sendCliText(`set pan ${pan}`);
        });
        document.getElementById('btnPing').addEventListener('click', () => {
            this.sendCliText('ping');
        });
        document.getElementById('btnGetInfo').addEventListener('click', () => {
            this.sendCliText('info');
        });

        // CLI command input
        const cliInput = document.getElementById('cliInput');
        const btnCliSend = document.getElementById('btnCliSend');

        if (cliInput && btnCliSend) {
            // Command history
            this.cliHistory = [];
            this.cliHistoryIndex = -1;

            btnCliSend.addEventListener('click', () => this.sendCliCommand());
            cliInput.addEventListener('keydown', (e) => {
                if (e.key === 'Enter') {
                    this.sendCliCommand();
                } else if (e.key === 'ArrowUp') {
                    e.preventDefault();
                    if (this.cliHistoryIndex < this.cliHistory.length - 1) {
                        this.cliHistoryIndex++;
                        cliInput.value = this.cliHistory[this.cliHistory.length - 1 - this.cliHistoryIndex];
                    }
                } else if (e.key === 'ArrowDown') {
                    e.preventDefault();
                    if (this.cliHistoryIndex > 0) {
                        this.cliHistoryIndex--;
                        cliInput.value = this.cliHistory[this.cliHistory.length - 1 - this.cliHistoryIndex];
                    } else if (this.cliHistoryIndex === 0) {
                        this.cliHistoryIndex = -1;
                        cliInput.value = '';
                    }
                }
            });
        }

        // Console clear button
        const btnClearConsole = document.getElementById('btnClearConsole');
        if (btnClearConsole) {
            btnClearConsole.addEventListener('click', () => {
                const container = document.getElementById('consoleOutput');
                container.innerHTML = '<span style="color: #666;">Console cleared</span>';
            });
        }

        // Connect WebSocket
        this.connectWebSocket();

        // Start render loop
        this.render();

        // Update rate counters
        setInterval(() => this.updateScanRate(), 1000);
        setInterval(() => this.updatePacketRate(), 1000);
    }

    updatePacketRate() {
        const rateEl = document.getElementById('packetRate');
        if (rateEl) {
            rateEl.textContent = (this.packetRateCounter || 0) + '/s';
        }
        this.packetRateCounter = 0;
    }

    connectWebSocket() {
        // Derive WebSocket port from HTTP port (HTTP port + 1)
        const httpPort = parseInt(window.location.port) || 8080;
        const wsPort = httpPort + 1;
        const wsUrl = `ws://${window.location.hostname}:${wsPort}/`;
        console.log('Connecting to', wsUrl);

        try {
            this.ws = new WebSocket(wsUrl);

            this.ws.onopen = () => {
                console.log('WebSocket connected');
                this.wsConnected = true;
                this.updateConnectionStatus();
            };

            this.ws.onclose = () => {
                console.log('WebSocket disconnected');
                this.wsConnected = false;
                this.updateConnectionStatus();
                // Reconnect after delay
                setTimeout(() => this.connectWebSocket(), 2000);
            };

            this.ws.onerror = (error) => {
                console.error('WebSocket error:', error);
            };

            this.ws.onmessage = (event) => {
                this.handleMessage(JSON.parse(event.data));
            };
        } catch (e) {
            console.error('WebSocket connection failed:', e);
            setTimeout(() => this.connectWebSocket(), 2000);
        }
    }

    handleMessage(data) {
        switch (data.type) {
            case 'spectrum':
                this.handleSpectrum(data);
                break;
            case 'history':
                this.handleHistory(data);
                break;
            case 'radio_info':
                this.handleRadioInfo(data.info);
                break;
            case 'rx_frame':
                this.handleRxFrame(data);
                break;
            case 'heartbeat':
                this.handleHeartbeat(data);
                break;
            case 'command_result':
                this.handleCommandResult(data);
                break;
            case 'debug':
                this.handleDebug(data);
                break;
        }
    }

    sendCommand(cmd, params = {}) {
        if (this.ws && this.wsConnected) {
            this.ws.send(JSON.stringify({
                type: 'command',
                cmd: cmd,
                params: params
            }));
        } else {
            console.warn('WebSocket not connected, cannot send command');
        }
    }

    sendCliText(text) {
        // Log to console
        const container = document.getElementById('consoleOutput');
        if (container) {
            this.clearConsolePlaceholder(container);
            const cmdDiv = document.createElement('div');
            cmdDiv.className = 'command-echo';
            cmdDiv.textContent = '> ' + text;
            container.appendChild(cmdDiv);
            this.trimAndScrollConsole(container);
        }
        // Send as cli_command
        this.sendCommand('cli_command', { text: text });
    }

    sendCliCommand() {
        const input = document.getElementById('cliInput');
        const text = input.value.trim();
        if (!text) return;

        // Add to history
        this.cliHistory.push(text);
        this.cliHistoryIndex = -1;

        // Clear input
        input.value = '';

        // Send via sendCliText
        this.sendCliText(text);
    }

    handleCommandResult(data) {
        console.log(`Command result:`, data.success ? data.result : data.error);
        const container = document.getElementById('consoleOutput');
        if (!container) return;

        const resultDiv = document.createElement('div');
        if (data.success && data.result) {
            resultDiv.className = 'command-result';
            resultDiv.textContent = data.result.output || '(no output)';
        } else if (data.error) {
            resultDiv.className = 'command-error';
            resultDiv.textContent = 'Error: ' + data.error;
        }
        container.appendChild(resultDiv);
        this.trimAndScrollConsole(container);
    }

    handleSpectrum(data) {
        this.currentSpectrum = {
            startCh: data.start_ch,
            endCh: data.end_ch,
            rssi: data.rssi,
            timestamp: data.timestamp,
            seq: data.seq
        };

        // Add to history
        this.spectrumHistory.push(this.currentSpectrum);
        if (this.spectrumHistory.length > this.maxHistory) {
            this.spectrumHistory.shift();
        }

        this.sampleCount++;
        this.rateCounter++;

        // Update UI
        this.updateCurrentSpectrum();
        this.updateStats();
    }

    handleHistory(data) {
        if (data.data && Array.isArray(data.data)) {
            this.spectrumHistory = data.data.map(d => ({
                startCh: d.start_ch,
                endCh: d.end_ch,
                rssi: d.rssi,
                timestamp: d.timestamp,
                seq: d.seq
            }));
            if (this.spectrumHistory.length > 0) {
                this.currentSpectrum = this.spectrumHistory[this.spectrumHistory.length - 1];
                this.updateCurrentSpectrum();
            }
        }
    }

    handleRadioInfo(info) {
        if (info.channel !== undefined) {
            document.getElementById('infoChannel').textContent = info.channel;
        }
        if (info.tx_power !== undefined) {
            document.getElementById('infoTxPower').textContent = info.tx_power + ' dBm';
        }
        if (info.pan_id !== undefined) {
            document.getElementById('infoPanId').textContent = '0x' + info.pan_id.toString(16).toUpperCase();
        }
        // Determine band from channel range
        if (info.channel_min !== undefined) {
            const band = info.channel_min >= 11 ? '2.4 GHz' : 'Sub-GHz';
            document.getElementById('infoBand').textContent = band;
        }
    }

    handleRxFrame(data) {
        const container = document.getElementById('packetsList');

        // Remove placeholder if present
        const placeholder = container.querySelector('.packet-placeholder');
        if (placeholder) {
            placeholder.remove();
        }

        // Create packet element
        const packet = document.createElement('div');
        packet.className = 'packet-item';

        const time = new Date(data.timestamp * 1000).toLocaleTimeString();
        packet.innerHTML = `
            <div class="packet-header">
                <span>${time}</span>
                <span>RSSI: ${data.rssi} dBm | LQI: ${data.lqi} | ${data.length} bytes</span>
            </div>
            <div class="packet-data">${data.data}</div>
        `;

        // Add to top
        container.insertBefore(packet, container.firstChild);

        // Limit to 50 packets
        while (container.children.length > 50) {
            container.removeChild(container.lastChild);
        }

        // Update packet count
        this.packetCount = (this.packetCount || 0) + 1;
        this.packetRateCounter = (this.packetRateCounter || 0) + 1;
        const countEl = document.getElementById('packetCount');
        if (countEl) countEl.textContent = this.packetCount;
    }

    handleHeartbeat(data) {
        // Could add heartbeat indicator or uptime display
        console.log(`Heartbeat: seq=${data.seq} uptime=${data.uptime}s`);
    }

    handleDebug(data) {
        const container = document.getElementById('consoleOutput');
        if (!container) return;

        this.clearConsolePlaceholder(container);

        // Parse and add each line
        const lines = data.text.split('\n').filter(line => line.trim());
        for (const line of lines) {
            const div = document.createElement('div');
            div.className = 'debug-line';

            // Color based on content
            if (line.includes('[INFO') || line.includes('INFO:')) {
                div.classList.add('info');
            } else if (line.includes('[WARN') || line.includes('WARN:')) {
                div.classList.add('warn');
            } else if (line.includes('[ERR') || line.includes('ERROR:')) {
                div.classList.add('error');
            }

            // Format timestamp
            const time = new Date(data.timestamp * 1000).toLocaleTimeString();
            div.innerHTML = `<span class="timestamp">[${time}]</span>${this.escapeHtml(line)}`;

            container.appendChild(div);
        }

        this.trimAndScrollConsole(container);
    }

    clearConsolePlaceholder(container) {
        const placeholder = container.querySelector('span');
        if (placeholder && (placeholder.textContent.includes('Waiting') || placeholder.textContent.includes('cleared'))) {
            container.innerHTML = '';
        }
    }

    trimAndScrollConsole(container) {
        // Auto-scroll to bottom
        container.scrollTop = container.scrollHeight;

        // Limit to 500 lines
        while (container.children.length > 500) {
            container.removeChild(container.firstChild);
        }
    }

    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    updateConnectionStatus() {
        const led = document.getElementById('wsLed');
        if (this.wsConnected) {
            led.className = 'led connected';
        } else {
            led.className = 'led disconnected';
        }
    }

    updateScanRate() {
        this.scanRate = this.rateCounter;
        document.getElementById('scanRate').textContent = this.rateCounter + ' scans/s';
        this.rateCounter = 0;
    }

    updateCurrentSpectrum() {
        if (!this.currentSpectrum) return;

        const container = document.getElementById('currentSpectrum');
        container.innerHTML = '';

        const { startCh, endCh, rssi } = this.currentSpectrum;

        for (let i = 0; i < rssi.length; i++) {
            const ch = startCh + i;
            const val = rssi[i];
            const pct = this.rssiToPercent(val);

            const row = document.createElement('div');
            row.className = 'channel-bar';
            row.innerHTML = `
                <span class="ch">${ch}</span>
                <div class="bar-container">
                    <div class="bar" style="width: ${pct}%"></div>
                </div>
                <span class="rssi">${val} dBm</span>
            `;
            container.appendChild(row);
        }
    }

    updateStats() {
        document.getElementById('sampleCount').textContent = this.sampleCount;

        if (this.currentSpectrum) {
            const rssi = this.currentSpectrum.rssi;
            const min = Math.min(...rssi);
            const max = Math.max(...rssi);
            const avg = Math.round(rssi.reduce((a, b) => a + b, 0) / rssi.length);

            document.getElementById('minRssi').textContent = min;
            document.getElementById('maxRssi').textContent = max;
            document.getElementById('avgRssi').textContent = avg;
        }
    }

    rssiToPercent(rssi) {
        // Map RSSI to percentage: -50 dBm (strong) = 100%, -130 dBm (weak) = 0%
        // Higher RSSI (less negative) = stronger signal = higher on screen
        const range = this.rssiMax - this.rssiMin;
        const pct = ((rssi - this.rssiMin) / range) * 100;
        return Math.max(0, Math.min(100, pct));
    }

    rssiToColor(rssi) {
        const pct = this.rssiToPercent(rssi) / 100;

        // Color gradient: blue -> green -> yellow -> red
        let r, g, b;
        if (pct < 0.25) {
            // Blue to cyan
            r = 0;
            g = Math.round(pct * 4 * 255);
            b = 255;
        } else if (pct < 0.5) {
            // Cyan to green
            r = 0;
            g = 255;
            b = Math.round((1 - (pct - 0.25) * 4) * 255);
        } else if (pct < 0.75) {
            // Green to yellow
            r = Math.round((pct - 0.5) * 4 * 255);
            g = 255;
            b = 0;
        } else {
            // Yellow to red
            r = 255;
            g = Math.round((1 - (pct - 0.75) * 4) * 255);
            b = 0;
        }

        return `rgb(${r}, ${g}, ${b})`;
    }

    setViewMode(mode) {
        this.viewMode = mode;
        document.getElementById('view2d').className = mode === '2d' ? 'active' : '';
        document.getElementById('view3d').className = mode === '3d' ? 'active' : '';

        // Show/hide appropriate container
        this.canvas.style.display = mode === '2d' ? 'block' : 'none';
        document.getElementById('plotly3d').style.display = mode === '3d' ? 'block' : 'none';

        if (mode === '3d') {
            this.initThree3D();
        }
    }

    initThree3D() {
        if (this.threeInitialized) return;

        const container = document.getElementById('plotly3d');
        const width = container.clientWidth;
        const height = container.clientHeight;

        // Scene
        this.threeScene = new THREE.Scene();
        this.threeScene.background = new THREE.Color(0x000000);

        // Camera - positioned above and in front, looking at origin
        this.threeCamera = new THREE.PerspectiveCamera(60, width / height, 0.1, 1000);
        this.threeCamera.position.set(0, -50, 40);  // In front (Y-), above (Z+)
        this.threeCamera.up.set(0, 0, 1);  // Z is up
        this.threeCamera.lookAt(0, 0, 0);

        // Renderer
        this.threeRenderer = new THREE.WebGLRenderer({ antialias: true });
        this.threeRenderer.setSize(width, height);
        this.threeRenderer.setPixelRatio(window.devicePixelRatio);
        container.appendChild(this.threeRenderer.domElement);

        // Lights
        const ambientLight = new THREE.AmbientLight(0xffffff, 0.6);
        this.threeScene.add(ambientLight);
        const directionalLight = new THREE.DirectionalLight(0xffffff, 0.4);
        directionalLight.position.set(0, -50, 50);
        this.threeScene.add(directionalLight);

        // Add axes
        this.createAxes();

        // Mouse controls - simple orbit around Z axis
        this.threeIsDragging = false;
        this.threeLastMouse = { x: 0, y: 0 };
        this.cameraAngle = 0;  // Angle around Z axis
        this.cameraHeight = 40;  // Height above surface
        this.cameraDistance = 50;  // Distance from center

        container.addEventListener('mousedown', (e) => {
            this.threeIsDragging = true;
            this.threeLastMouse = { x: e.clientX, y: e.clientY };
        });

        container.addEventListener('mousemove', (e) => {
            if (this.threeIsDragging) {
                const dx = e.clientX - this.threeLastMouse.x;
                const dy = e.clientY - this.threeLastMouse.y;
                this.cameraAngle += dx * 0.01;
                this.cameraHeight = Math.max(10, Math.min(80, this.cameraHeight + dy * 0.3));
                this.updateCameraPosition();
                this.threeLastMouse = { x: e.clientX, y: e.clientY };
            }
        });

        container.addEventListener('mouseup', () => { this.threeIsDragging = false; });
        container.addEventListener('mouseleave', () => { this.threeIsDragging = false; });

        // Mouse wheel for zoom
        container.addEventListener('wheel', (e) => {
            e.preventDefault();
            this.cameraDistance = Math.max(30, Math.min(150, this.cameraDistance + e.deltaY * 0.1));
            this.updateCameraPosition();
        });

        // Handle resize
        window.addEventListener('resize', () => {
            const w = container.clientWidth;
            const h = container.clientHeight;
            this.threeCamera.aspect = w / h;
            this.threeCamera.updateProjectionMatrix();
            this.threeRenderer.setSize(w, h);
        });

        this.threeInitialized = true;
        this.updateCameraPosition();
    }

    updateCameraPosition() {
        // Orbit camera around Z axis at given height and distance
        this.threeCamera.position.x = this.cameraDistance * Math.sin(this.cameraAngle);
        this.threeCamera.position.y = -this.cameraDistance * Math.cos(this.cameraAngle);
        this.threeCamera.position.z = this.cameraHeight;
        this.threeCamera.lookAt(0, 0, 0);
    }

    createAxes() {
        const axisColor = 0x888888;
        const axisMaterial = new THREE.LineBasicMaterial({ color: axisColor });

        // X axis (channels) - at front bottom, red tint
        const xGeom = new THREE.BufferGeometry().setFromPoints([
            new THREE.Vector3(-25, -20, -10),
            new THREE.Vector3(25, -20, -10)
        ]);
        const xAxis = new THREE.Line(xGeom, new THREE.LineBasicMaterial({ color: 0xff6666 }));
        this.threeScene.add(xAxis);

        // Y axis (time) - on left side, green tint
        const yGeom = new THREE.BufferGeometry().setFromPoints([
            new THREE.Vector3(-25, -20, -10),
            new THREE.Vector3(-25, 20, -10)
        ]);
        const yAxis = new THREE.Line(yGeom, new THREE.LineBasicMaterial({ color: 0x66ff66 }));
        this.threeScene.add(yAxis);

        // Z axis (RSSI) - at back left corner, blue tint
        const zGeom = new THREE.BufferGeometry().setFromPoints([
            new THREE.Vector3(-25, 20, -10),
            new THREE.Vector3(-25, 20, 20)
        ]);
        const zAxis = new THREE.Line(zGeom, new THREE.LineBasicMaterial({ color: 0x6666ff }));
        this.threeScene.add(zAxis);

        // Add tick marks on X axis (channels) - at front bottom
        const tickPositions = [-25, -12.5, 0, 12.5, 25];
        for (let i = 0; i < tickPositions.length; i++) {
            const x = tickPositions[i];
            const tickGeom = new THREE.BufferGeometry().setFromPoints([
                new THREE.Vector3(x, -20, -10),
                new THREE.Vector3(x, -22, -10)
            ]);
            const tick = new THREE.Line(tickGeom, axisMaterial);
            this.threeScene.add(tick);
        }

        // Add tick marks on Z axis (RSSI) - at back
        for (let i = -10; i <= 20; i += 10) {
            const tickGeom = new THREE.BufferGeometry().setFromPoints([
                new THREE.Vector3(-25, 20, i),
                new THREE.Vector3(-23, 20, i)
            ]);
            const tick = new THREE.Line(tickGeom, axisMaterial);
            this.threeScene.add(tick);
        }

        // Create text labels using sprites
        this.createTextSprite('Channel', 0, -28, -10, 0xff6666);
        this.createTextSprite('Time', -30, 0, -12, 0x66ff66);
        this.createTextSprite('RSSI', -30, 22, 10, 0x6666ff);
    }

    createTextSprite(text, x, y, z, color) {
        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');
        canvas.width = 128;
        canvas.height = 32;

        ctx.fillStyle = '#' + color.toString(16).padStart(6, '0');
        ctx.font = 'bold 18px Arial';
        ctx.textAlign = 'center';
        ctx.fillText(text, 64, 22);

        const texture = new THREE.CanvasTexture(canvas);
        const spriteMaterial = new THREE.SpriteMaterial({ map: texture });
        const sprite = new THREE.Sprite(spriteMaterial);
        sprite.position.set(x, y, z);
        sprite.scale.set(12, 3, 1);
        sprite.userData = { canvas, ctx, color };  // Store for updates
        this.threeScene.add(sprite);
        return sprite;
    }

    rssiToHeight(rssi) {
        // Map RSSI (-130 to -50) to height (0 to 30)
        return ((rssi - this.rssiMin) / (this.rssiMax - this.rssiMin)) * 30;
    }

    rssiToColorThree(rssi) {
        const pct = Math.max(0, Math.min(1, (rssi - this.rssiMin) / (this.rssiMax - this.rssiMin)));

        // Custom colormap: dark blue -> cyan -> green -> yellow -> red
        const colormap = [
            [0.0, 0.0, 0.3],    // 0.0 - dark blue
            [0.0, 0.2, 0.6],    // 0.15 - blue
            [0.0, 0.6, 0.8],    // 0.3 - cyan
            [0.0, 0.8, 0.4],    // 0.45 - green-cyan
            [0.2, 0.9, 0.2],    // 0.6 - green
            [0.6, 0.9, 0.1],    // 0.7 - yellow-green
            [1.0, 0.9, 0.0],    // 0.8 - yellow
            [1.0, 0.5, 0.0],    // 0.9 - orange
            [1.0, 0.0, 0.0]     // 1.0 - red
        ];

        // Interpolate between colormap entries
        const idx = pct * (colormap.length - 1);
        const i = Math.floor(idx);
        const t = idx - i;

        const c1 = colormap[Math.min(i, colormap.length - 1)];
        const c2 = colormap[Math.min(i + 1, colormap.length - 1)];

        const r = c1[0] + t * (c2[0] - c1[0]);
        const g = c1[1] + t * (c2[1] - c1[1]);
        const b = c1[2] + t * (c2[2] - c1[2]);

        return new THREE.Color(r, g, b);
    }

    updateThree3D() {
        if (!this.threeInitialized || this.spectrumHistory.length < 2) return;

        const data = this.spectrumHistory.slice(-this.maxHistory);
        const numSamples = data.length;
        const numChannels = data[0].rssi.length;

        // Remove old mesh
        if (this.threeMesh) {
            this.threeScene.remove(this.threeMesh);
            this.threeMesh.geometry.dispose();
            this.threeMesh.material.dispose();
        }

        // Fixed size geometry centered at origin
        // X = channels (left to right): -25 to +25
        // Y = time (back to front): +20 (back/new) to -20 (front/old)
        // Z = RSSI height: 0 to 20
        const WIDTH = 50;   // X dimension (channels)
        const DEPTH = 40;   // Y dimension (time)

        const geometry = new THREE.PlaneGeometry(
            WIDTH, DEPTH,
            numChannels - 1, numSamples - 1
        );

        const positions = geometry.attributes.position.array;
        const colors = new Float32Array(positions.length);

        // PlaneGeometry creates vertices row by row (Y varies slower, X varies faster)
        // Row 0 is at Y = -DEPTH/2 (front), Row N is at Y = +DEPTH/2 (back)
        // We want: newest data at back, scrolls towards you, oldest falls off at front
        for (let row = 0; row < numSamples; row++) {
            for (let col = 0; col < numChannels; col++) {
                const vertexIndex = row * numChannels + col;

                // Flip time: row 0 (front) = newest, row N (back) = oldest
                const timeIndex = numSamples - 1 - row;
                const channelIndex = col;

                const rssi = data[timeIndex].rssi[channelIndex];
                const height = this.rssiToHeight(rssi);

                // Set Z position (height)
                positions[vertexIndex * 3 + 2] = height;

                // Color
                const color = this.rssiToColorThree(rssi);
                colors[vertexIndex * 3] = color.r;
                colors[vertexIndex * 3 + 1] = color.g;
                colors[vertexIndex * 3 + 2] = color.b;
            }
        }

        geometry.setAttribute('color', new THREE.BufferAttribute(colors, 3));
        geometry.attributes.position.needsUpdate = true;
        geometry.computeVertexNormals();

        // Material with vertex colors
        const material = new THREE.MeshPhongMaterial({
            vertexColors: true,
            side: THREE.DoubleSide,
            flatShading: false,
            specular: 0x000000,
            shininess: 0
        });

        this.threeMesh = new THREE.Mesh(geometry, material);
        // Move down slightly so peaks are more visible
        this.threeMesh.position.set(0, 0, -10);
        this.threeScene.add(this.threeMesh);
    }

    resizeCanvas() {
        const container = this.canvas.parentElement;
        const rect = container.getBoundingClientRect();
        const dpr = window.devicePixelRatio || 1;

        // Set canvas size accounting for device pixel ratio for crisp rendering
        this.canvas.width = rect.width * dpr;
        this.canvas.height = rect.height * dpr;

        // Scale context for high DPI displays
        this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

        // Store logical dimensions for drawing
        this.canvasWidth = rect.width;
        this.canvasHeight = rect.height;
    }

    render() {
        if (this.viewMode === '2d') {
            const w = this.canvasWidth || this.canvas.width;
            const h = this.canvasHeight || this.canvas.height;

            this.ctx.fillStyle = '#0d1117';
            this.ctx.fillRect(0, 0, w, h);

            this.render2D();
        }
        // 3D is handled by Plotly via updatePlotly3D()

        requestAnimationFrame(() => this.render());
    }

    render2D() {
        if (!this.currentSpectrum) return;

        const { rssi, startCh } = this.currentSpectrum;
        const numChannels = rssi.length;

        const cw = this.canvasWidth || this.canvas.width;
        const ch = this.canvasHeight || this.canvas.height;

        const padding = { top: 40, right: 40, bottom: 50, left: 60 };
        const width = cw - padding.left - padding.right;
        const height = ch - padding.top - padding.bottom;

        const barWidth = width / numChannels - 4;

        // Draw grid
        this.ctx.strokeStyle = '#1a2744';
        this.ctx.lineWidth = 1;

        // Horizontal grid lines (RSSI levels)
        for (let rssi = this.rssiMin; rssi <= this.rssiMax; rssi += 10) {
            const y = padding.top + height * (1 - (rssi - this.rssiMin) / (this.rssiMax - this.rssiMin));
            this.ctx.beginPath();
            this.ctx.moveTo(padding.left, y);
            this.ctx.lineTo(padding.left + width, y);
            this.ctx.stroke();

            // Label
            this.ctx.fillStyle = '#666';
            this.ctx.font = '11px sans-serif';
            this.ctx.textAlign = 'right';
            this.ctx.fillText(rssi + ' dBm', padding.left - 5, y + 4);
        }

        // Draw bars
        for (let i = 0; i < numChannels; i++) {
            const val = rssi[i];
            const pct = this.rssiToPercent(val) / 100;
            const barHeight = pct * height;

            const x = padding.left + i * (width / numChannels) + 2;
            const y = padding.top + height - barHeight;

            // Bar gradient
            const gradient = this.ctx.createLinearGradient(x, y + barHeight, x, y);
            gradient.addColorStop(0, this.rssiToColor(this.rssiMin));
            gradient.addColorStop(1, this.rssiToColor(val));

            this.ctx.fillStyle = gradient;
            this.ctx.fillRect(x, y, barWidth, barHeight);

            // Channel label
            this.ctx.fillStyle = '#888';
            this.ctx.font = '10px sans-serif';
            this.ctx.textAlign = 'center';
            this.ctx.fillText(startCh + i, x + barWidth / 2, padding.top + height + 15);
        }

        // X axis label
        this.ctx.fillStyle = '#888';
        this.ctx.font = '12px sans-serif';
        this.ctx.textAlign = 'center';
        this.ctx.fillText('Channel', padding.left + width / 2, ch - 10);

        // Y axis label
        this.ctx.save();
        this.ctx.translate(15, padding.top + height / 2);
        this.ctx.rotate(-Math.PI / 2);
        this.ctx.fillText('RSSI (dBm)', 0, 0);
        this.ctx.restore();

        // Title
        this.ctx.fillStyle = '#fff';
        this.ctx.font = '14px sans-serif';
        this.ctx.textAlign = 'left';
        this.ctx.fillText('Real-time Spectrum', padding.left, 25);
    }
}

// Initialize when page loads
window.addEventListener('DOMContentLoaded', () => {
    window.analyzer = new SpectrumAnalyzer();
});

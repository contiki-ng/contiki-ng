/**
 * Packet Analyzer for Serial Radio
 *
 * Captures, displays, and analyzes IEEE 802.15.4 packets.
 */

class PacketAnalyzer {
    constructor() {
        // WebSocket
        this.ws = null;
        this.wsConnected = false;

        // Sniffing state
        this.isSniffing = false;
        this.isJamming = false;

        // Packet storage
        this.packets = [];
        this.maxPackets = 1000;
        this.selectedPacket = null;

        // Statistics
        this.stats = {
            total: 0,
            beacons: 0,
            data: 0,
            ack: 0,
            command: 0,
            rssiSum: 0
        };

        // Rate tracking
        this.rateCounter = 0;
        this.packetRate = 0;

        // Filter
        this.filter = '';

        this.init();
    }

    init() {
        // Setup buttons
        document.getElementById('btnSniffStart').addEventListener('click', () => this.startSniffing());
        document.getElementById('btnSniffStop').addEventListener('click', () => this.stopSniffing());
        document.getElementById('btnClear').addEventListener('click', () => this.clearPackets());

        // Setup filter
        document.getElementById('filterInput').addEventListener('input', (e) => {
            this.filter = e.target.value.toLowerCase();
            this.renderPacketList();
        });

        // Setup jam buttons
        document.getElementById('btnJamStart').addEventListener('click', () => {
            const channel = parseInt(document.getElementById('jamChannel').value);
            const interval = parseInt(document.getElementById('jamInterval').value);
            // The server only executes CLI text (cmd='cli_command'); map UI
            // actions to the equivalent CLI command.
            this.sendCommand('cli_command', { text: `jam start ${channel} ${interval}` });
            this.isJamming = true;
            this.updateJammingStatus();
        });
        document.getElementById('btnJamStop').addEventListener('click', () => {
            this.sendCommand('cli_command', { text: 'jam stop' });
            this.isJamming = false;
            this.updateJammingStatus();
        });

        // Connect WebSocket
        this.connectWebSocket();

        // Update rate counter
        setInterval(() => this.updatePacketRate(), 1000);
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
                this.isSniffing = false;
                this.updateConnectionStatus();
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
            case 'rx_frame':
                this.handleRxFrame(data);
                break;
            case 'command_result':
                this.handleCommandResult(data);
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
        }
    }

    handleCommandResult(data) {
        console.log(`Command '${data.cmd}' result:`, data.success ? data.result : data.error);
    }

    startSniffing() {
        // The server only executes CLI text; "sniff on" enables RX on the
        // device and starts streaming RX_FRAME events to the web clients.
        this.sendCommand('cli_command', { text: 'sniff on' });
        this.isSniffing = true;
        this.updateSniffingStatus();
    }

    stopSniffing() {
        this.sendCommand('cli_command', { text: 'sniff off' });
        this.isSniffing = false;
        this.updateSniffingStatus();
    }

    handleRxFrame(data) {
        const packet = {
            id: this.stats.total,
            timestamp: data.timestamp,
            rssi: data.rssi,
            lqi: data.lqi,
            length: data.length,
            data: data.data,  // hex string
            bytes: this.hexToBytes(data.data),
            parsed: this.parseFrame(this.hexToBytes(data.data))
        };

        // Update statistics
        this.stats.total++;
        this.stats.rssiSum += data.rssi;
        this.rateCounter++;

        if (packet.parsed) {
            switch (packet.parsed.frameType) {
                case 0: this.stats.beacon++; break;
                case 1: this.stats.data++; break;
                case 2: this.stats.ack++; break;
                case 3: this.stats.command++; break;
            }
        }

        // Store packet
        this.packets.unshift(packet);
        if (this.packets.length > this.maxPackets) {
            this.packets.pop();
        }

        this.updateStats();
        this.addPacketToList(packet);
    }

    hexToBytes(hex) {
        const bytes = [];
        for (let i = 0; i < hex.length; i += 2) {
            bytes.push(parseInt(hex.substr(i, 2), 16));
        }
        return bytes;
    }

    parseFrame(bytes) {
        if (bytes.length < 2) return null;

        // IEEE 802.15.4 Frame Control Field
        const fcf = bytes[0] | (bytes[1] << 8);

        const frameType = fcf & 0x07;
        const securityEnabled = (fcf >> 3) & 0x01;
        const framePending = (fcf >> 4) & 0x01;
        const ackRequest = (fcf >> 5) & 0x01;
        const panIdCompression = (fcf >> 6) & 0x01;
        const destAddrMode = (fcf >> 10) & 0x03;
        const frameVersion = (fcf >> 12) & 0x03;
        const srcAddrMode = (fcf >> 14) & 0x03;

        const frameTypeNames = ['Beacon', 'Data', 'ACK', 'Command', 'Reserved', 'Multipurpose', 'Fragment', 'Extended'];
        const addrModeNames = ['None', 'Reserved', 'Short (16-bit)', 'Extended (64-bit)'];

        const parsed = {
            frameType: frameType,
            frameTypeName: frameTypeNames[frameType] || 'Unknown',
            securityEnabled: securityEnabled,
            framePending: framePending,
            ackRequest: ackRequest,
            panIdCompression: panIdCompression,
            destAddrMode: destAddrMode,
            destAddrModeName: addrModeNames[destAddrMode],
            frameVersion: frameVersion,
            srcAddrMode: srcAddrMode,
            srcAddrModeName: addrModeNames[srcAddrMode]
        };

        let offset = 2;

        // Sequence number (not present in some frame versions)
        if (bytes.length > offset) {
            parsed.seqNum = bytes[offset];
            offset++;
        }

        // Destination PAN ID
        if (destAddrMode !== 0 && bytes.length >= offset + 2) {
            parsed.destPanId = bytes[offset] | (bytes[offset + 1] << 8);
            offset += 2;
        }

        // Destination Address
        if (destAddrMode === 2 && bytes.length >= offset + 2) {
            parsed.destAddr = bytes[offset] | (bytes[offset + 1] << 8);
            parsed.destAddrStr = '0x' + parsed.destAddr.toString(16).padStart(4, '0').toUpperCase();
            offset += 2;
        } else if (destAddrMode === 3 && bytes.length >= offset + 8) {
            parsed.destAddrExt = bytes.slice(offset, offset + 8);
            parsed.destAddrStr = parsed.destAddrExt.map(b => b.toString(16).padStart(2, '0')).join(':').toUpperCase();
            offset += 8;
        }

        // Source PAN ID (if not compressed)
        if (srcAddrMode !== 0 && !panIdCompression && bytes.length >= offset + 2) {
            parsed.srcPanId = bytes[offset] | (bytes[offset + 1] << 8);
            offset += 2;
        } else if (srcAddrMode !== 0 && panIdCompression) {
            parsed.srcPanId = parsed.destPanId;
        }

        // Source Address
        if (srcAddrMode === 2 && bytes.length >= offset + 2) {
            parsed.srcAddr = bytes[offset] | (bytes[offset + 1] << 8);
            parsed.srcAddrStr = '0x' + parsed.srcAddr.toString(16).padStart(4, '0').toUpperCase();
            offset += 2;
        } else if (srcAddrMode === 3 && bytes.length >= offset + 8) {
            parsed.srcAddrExt = bytes.slice(offset, offset + 8);
            parsed.srcAddrStr = parsed.srcAddrExt.map(b => b.toString(16).padStart(2, '0')).join(':').toUpperCase();
            offset += 8;
        }

        // Payload starts after header
        parsed.headerLength = offset;
        parsed.payloadLength = bytes.length - offset - 2; // -2 for FCS
        if (parsed.payloadLength > 0) {
            parsed.payload = bytes.slice(offset, bytes.length - 2);
        }

        return parsed;
    }

    matchesFilter(packet) {
        if (!this.filter) return true;

        const f = this.filter;

        // RSSI filter: rssi>-80 or rssi<-50
        const rssiMatch = f.match(/rssi\s*([<>]=?)\s*(-?\d+)/);
        if (rssiMatch) {
            const op = rssiMatch[1];
            const val = parseInt(rssiMatch[2]);
            switch (op) {
                case '>': return packet.rssi > val;
                case '>=': return packet.rssi >= val;
                case '<': return packet.rssi < val;
                case '<=': return packet.rssi <= val;
            }
        }

        // Length filter: len>10 or len<50
        const lenMatch = f.match(/len\s*([<>]=?)\s*(\d+)/);
        if (lenMatch) {
            const op = lenMatch[1];
            const val = parseInt(lenMatch[2]);
            switch (op) {
                case '>': return packet.length > val;
                case '>=': return packet.length >= val;
                case '<': return packet.length < val;
                case '<=': return packet.length <= val;
            }
        }

        // Hex filter: hex:41424344
        const hexMatch = f.match(/hex:([0-9a-f]+)/i);
        if (hexMatch) {
            return packet.data.toLowerCase().includes(hexMatch[1].toLowerCase());
        }

        // Type filter: beacon, data, ack, command
        if (packet.parsed) {
            if (f.includes('beacon') && packet.parsed.frameType === 0) return true;
            if (f.includes('data') && packet.parsed.frameType === 1) return true;
            if (f.includes('ack') && packet.parsed.frameType === 2) return true;
            if (f.includes('command') && packet.parsed.frameType === 3) return true;
        }

        // Address filter
        if (packet.parsed && packet.parsed.srcAddrStr) {
            if (packet.parsed.srcAddrStr.toLowerCase().includes(f)) return true;
        }
        if (packet.parsed && packet.parsed.destAddrStr) {
            if (packet.parsed.destAddrStr.toLowerCase().includes(f)) return true;
        }

        // General text search in hex data
        if (packet.data.toLowerCase().includes(f)) return true;

        return false;
    }

    addPacketToList(packet) {
        const container = document.getElementById('packetsList');

        // Remove "no packets" message
        const noPackets = container.querySelector('.no-packets');
        if (noPackets) {
            noPackets.remove();
        }

        // Check filter
        if (!this.matchesFilter(packet)) return;

        const row = this.createPacketRow(packet);
        container.insertBefore(row, container.firstChild);

        // Limit displayed packets
        while (container.children.length > 200) {
            container.removeChild(container.lastChild);
        }
    }

    createPacketRow(packet) {
        const row = document.createElement('div');
        row.className = 'packet-row';
        row.dataset.packetId = packet.id;

        const time = new Date(packet.timestamp * 1000).toLocaleTimeString();
        const rssiClass = packet.rssi < -90 ? 'very-weak' : (packet.rssi < -70 ? 'weak' : '');
        const frameType = packet.parsed ? packet.parsed.frameTypeName : 'Unknown';

        row.innerHTML = `
            <span class="time">${time}</span>
            <span class="rssi ${rssiClass}">${packet.rssi}</span>
            <span class="lqi">${packet.lqi}</span>
            <span class="len">${packet.length}</span>
            <span class="type">${frameType}</span>
        `;

        row.addEventListener('click', () => this.selectPacket(packet, row));

        return row;
    }

    selectPacket(packet, row) {
        // Remove previous selection
        const prev = document.querySelector('.packet-row.selected');
        if (prev) prev.classList.remove('selected');

        // Select new
        row.classList.add('selected');
        this.selectedPacket = packet;

        this.showPacketDetails(packet);
        this.showHexDump(packet);
    }

    showPacketDetails(packet) {
        const container = document.getElementById('packetDetail');

        let html = '<div class="detail-section">';
        html += '<h4>Frame Info</h4>';
        html += `<div class="detail-row"><span class="label">Time:</span><span class="value">${new Date(packet.timestamp * 1000).toLocaleString()}</span></div>`;
        html += `<div class="detail-row"><span class="label">RSSI:</span><span class="value">${packet.rssi} dBm</span></div>`;
        html += `<div class="detail-row"><span class="label">LQI:</span><span class="value">${packet.lqi}</span></div>`;
        html += `<div class="detail-row"><span class="label">Length:</span><span class="value">${packet.length} bytes</span></div>`;
        html += '</div>';

        if (packet.parsed) {
            const p = packet.parsed;
            html += '<div class="detail-section">';
            html += '<h4>IEEE 802.15.4 Header</h4>';
            html += `<div class="detail-row"><span class="label">Frame Type:</span><span class="value">${p.frameTypeName}</span></div>`;
            html += `<div class="detail-row"><span class="label">Seq Number:</span><span class="value">${p.seqNum !== undefined ? p.seqNum : 'N/A'}</span></div>`;
            html += `<div class="detail-row"><span class="label">Security:</span><span class="value">${p.securityEnabled ? 'Yes' : 'No'}</span></div>`;
            html += `<div class="detail-row"><span class="label">ACK Request:</span><span class="value">${p.ackRequest ? 'Yes' : 'No'}</span></div>`;
            html += '</div>';

            if (p.destPanId !== undefined || p.destAddrStr) {
                html += '<div class="detail-section">';
                html += '<h4>Destination</h4>';
                if (p.destPanId !== undefined) {
                    html += `<div class="detail-row"><span class="label">PAN ID:</span><span class="value">0x${p.destPanId.toString(16).padStart(4, '0').toUpperCase()}</span></div>`;
                }
                if (p.destAddrStr) {
                    html += `<div class="detail-row"><span class="label">Address:</span><span class="value">${p.destAddrStr}</span></div>`;
                }
                html += '</div>';
            }

            if (p.srcPanId !== undefined || p.srcAddrStr) {
                html += '<div class="detail-section">';
                html += '<h4>Source</h4>';
                if (p.srcPanId !== undefined) {
                    html += `<div class="detail-row"><span class="label">PAN ID:</span><span class="value">0x${p.srcPanId.toString(16).padStart(4, '0').toUpperCase()}</span></div>`;
                }
                if (p.srcAddrStr) {
                    html += `<div class="detail-row"><span class="label">Address:</span><span class="value">${p.srcAddrStr}</span></div>`;
                }
                html += '</div>';
            }

            if (p.payloadLength > 0) {
                html += '<div class="detail-section">';
                html += '<h4>Payload</h4>';
                html += `<div class="detail-row"><span class="label">Length:</span><span class="value">${p.payloadLength} bytes</span></div>`;
                html += '</div>';
            }
        }

        container.innerHTML = html;
    }

    showHexDump(packet) {
        const container = document.getElementById('hexDump');
        const bytes = packet.bytes;

        let html = '';
        for (let i = 0; i < bytes.length; i += 16) {
            const offset = i.toString(16).padStart(4, '0');
            const hexPart = [];
            const asciiPart = [];

            for (let j = 0; j < 16; j++) {
                if (i + j < bytes.length) {
                    const b = bytes[i + j];
                    hexPart.push(b.toString(16).padStart(2, '0'));
                    asciiPart.push(b >= 32 && b < 127 ? String.fromCharCode(b) : '.');
                } else {
                    hexPart.push('  ');
                    asciiPart.push(' ');
                }
            }

            // Escape the ASCII column: packet bytes are attacker-influenced and
            // can contain '<', '>', '&', which would otherwise be injected into
            // innerHTML below.
            const asciiStr = asciiPart.join('')
                .replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
            html += `<span class="offset">${offset}</span>  ${hexPart.slice(0, 8).join(' ')}  ${hexPart.slice(8).join(' ')}  <span class="ascii">|${asciiStr}|</span>\n`;
        }

        container.innerHTML = html || 'No data';
    }

    renderPacketList() {
        const container = document.getElementById('packetsList');
        container.innerHTML = '';

        const filtered = this.packets.filter(p => this.matchesFilter(p));

        if (filtered.length === 0) {
            container.innerHTML = '<div class="no-packets">No packets match filter</div>';
            return;
        }

        for (const packet of filtered.slice(0, 200)) {
            const row = this.createPacketRow(packet);
            container.appendChild(row);
        }
    }

    clearPackets() {
        this.packets = [];
        this.stats = {
            total: 0,
            beacons: 0,
            data: 0,
            ack: 0,
            command: 0,
            rssiSum: 0
        };
        this.selectedPacket = null;

        document.getElementById('packetsList').innerHTML = '<div class="no-packets">No packets captured</div>';
        document.getElementById('packetDetail').innerHTML = '<div class="no-packets">Select a packet to view details</div>';
        document.getElementById('hexDump').textContent = 'Select a packet to view hex dump';

        this.updateStats();
    }

    updateStats() {
        document.getElementById('totalPackets').textContent = this.stats.total;
        document.getElementById('beaconCount').textContent = this.stats.beacons || 0;
        document.getElementById('dataCount').textContent = this.stats.data || 0;

        const avgRssi = this.stats.total > 0 ? Math.round(this.stats.rssiSum / this.stats.total) : '--';
        document.getElementById('avgRssi').textContent = avgRssi;
    }

    updatePacketRate() {
        this.packetRate = this.rateCounter;
        document.getElementById('packetRate').textContent = this.rateCounter + ' pkt/s';
        this.rateCounter = 0;
    }

    updateConnectionStatus() {
        const led = document.getElementById('wsLed');
        led.className = this.wsConnected ? 'led connected' : 'led disconnected';
    }

    updateSniffingStatus() {
        const led = document.getElementById('sniffLed');
        led.className = this.isSniffing ? 'led sniffing' : 'led';

        document.getElementById('btnSniffStart').disabled = this.isSniffing;
        document.getElementById('btnSniffStop').disabled = !this.isSniffing;
    }

    updateJammingStatus() {
        document.getElementById('btnJamStart').disabled = this.isJamming;
        document.getElementById('btnJamStop').disabled = !this.isJamming;

        // Visual feedback - make button red when jamming
        if (this.isJamming) {
            document.getElementById('btnJamStart').classList.add('active');
            document.getElementById('btnJamStart').textContent = 'Jamming...';
        } else {
            document.getElementById('btnJamStart').classList.remove('active');
            document.getElementById('btnJamStart').textContent = 'Start Jam';
        }
    }
}

// Initialize when page loads
window.addEventListener('DOMContentLoaded', () => {
    window.analyzer = new PacketAnalyzer();
});

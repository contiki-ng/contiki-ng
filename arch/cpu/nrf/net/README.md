# nRF5340 Inter-Processor Communication (IPC)

The nRF5340 SoC has two ARM Cortex-M33 cores: an **application core**
and a **network core**. The network core has exclusive access to the
802.15.4 radio hardware. This IPC subsystem allows the application core
to use the radio by forwarding all radio operations to the network core
over shared memory.

## Architecture

```
 Application Core                         Network Core
 +--------------------------+             +---------------------------+
 |  Contiki-NG application  |             |  ipc-radio-service        |
 |  (RPL, UDP, CoAP, ...)   |             |  (bare-metal radio proxy) |
 |                          |             |                           |
 |  NETSTACK_RADIO =        |   shared    |  NETSTACK_RADIO =         |
 |    ipc_radio_driver  ----+--> memory --+-->  nrf_ieee_driver       |
 |                          |  0x20070000 |                           |
 |  drain_net_log() <-------+-- log ring -+--- dbg_putchar()         |
 +--------------------------+   buffer    +---------------------------+
```

The application core runs a full Contiki-NG stack (IPv6, RPL, MAC) with
`ipc_radio_driver` as the radio driver. Each radio API call
(init, send, CCA, get/set value, etc.) is serialized into a command,
written to shared memory, and signalled to the network core via the
hardware IPC peripheral.

The network core runs a minimal Contiki-NG image with the IPC MAC
driver (`ipc_mac_driver`) which forwards received 802.15.4 frames
to the application core via shared memory. IPC commands from the
application core are handled by the `ipc-radio-service` process.

**Note:** TSCH is not supported over the IPC radio driver. The
synchronous command/response design and the latency of crossing the
IPC boundary are incompatible with TSCH's tight slot timing. Use CSMA
(the default MAC layer).

## Shared Memory Layout

The shared memory region is placed at `0x20070000` in the application
core's RAM1, which is accessible from both cores. The layout is defined
in `nrf-ipc.h`:

| Field         | Direction       | Purpose                                 |
|---------------|-----------------|-----------------------------------------|
| `version`     | --              | Protocol version for compatibility      |
| `net_ready`   | net -> app      | Set to 1 when the net core has booted   |
| `cmd` / `cmd_pending` | app -> net | Command mailbox (synchronous)     |
| `rsp` / `rsp_ready`   | net -> app | Response mailbox (synchronous)    |
| `rx`          | net -> app      | Received 802.15.4 frame (asynchronous)  |
| `log`         | net -> app      | Debug output ring buffer (2 KB)         |

### Command/Response Protocol

Communication is synchronous: the application core writes a command,
sets `cmd_pending = 1`, triggers an IPC signal, and busy-waits until
`rsp_ready` is set by the network core. Memory barriers (`__DMB()`)
ensure correct ordering across cores.

Supported commands (`enum nrf_ipc_cmd_type`):

| Command              | Description                              |
|----------------------|------------------------------------------|
| `NRF_IPC_CMD_INIT`   | Initialize the radio                    |
| `NRF_IPC_CMD_ON/OFF` | Enable/disable the radio                |
| `NRF_IPC_CMD_SEND`   | Transmit a frame                        |
| `NRF_IPC_CMD_CCA`    | Clear Channel Assessment                |
| `NRF_IPC_CMD_RECEIVING` | Check if a frame is being received   |
| `NRF_IPC_CMD_PENDING`   | Check for pending received frames    |
| `NRF_IPC_CMD_GET/SET_VALUE`  | Get/set a radio parameter (int) |
| `NRF_IPC_CMD_GET/SET_OBJECT` | Get/set a radio parameter (blob)|
| `NRF_IPC_CMD_DIAG`   | Read radio diagnostic registers         |

### Frame Reception

Received frames flow asynchronously from the network core via the
interrupt-driven IPC MAC:

1. The radio ISR fires on CRCOK and polls the radio driver process.
2. The radio driver process reads the frame into packetbuf and calls
   `NETSTACK_MAC.input()`, which is the IPC MAC's `packet_input()`.
3. The IPC MAC sends a software ACK if needed, writes the frame into
   `shm->rx` with RSSI and LQI metadata, sets `shm->rx.pending = 1`,
   and sends an IPC signal.
4. On the app core, `ipc_radio_pending_packet()` checks `shm->rx.pending`
   and copies the frame into a local buffer. The `ipc_radio_process`
   also delivers frames to the MAC layer via `NETSTACK_MAC.input()`.

Between events, the network core sleeps (WFI in `platform_idle()`),
minimizing energy consumption and preventing bus stalls.

### 802.15.4 ACK Handling

The IPC MAC sends software ACKs for received frames that have the
ACK Request bit set. ACKs are transmitted without CCA (as required
by 802.15.4). The ACK is sent from within the radio driver's process
context, which runs immediately after the radio ISR — well within
the ~400 us ACK timing window.

## Log Forwarding

Both cores share the same UART pins (P0.20/P0.22), so only one core
can drive the UART at a time. The solution is:

- The **network core** has UARTE disabled (`NRF_HAS_UARTE 0`). Its
  `dbg_putchar()` (in `arch/cpu/nrf/os/dbg-arch.c`) writes to a
  shared memory ring buffer instead of the UART.
- The **application core** periodically drains this ring buffer and
  prints each line with a `[NET]` prefix via its own UART.

This produces clean, interleaved output from both cores on a single
serial port.

## IPC Transport Layer

The hardware IPC peripheral provides signalling between cores:

- **Channel 0**: application core -> network core
- **Channel 1**: network core -> application core

`nrf_ipc_init()` configures the send/receive channel mapping based on
which core is running. `nrf_ipc_signal()` triggers `TASKS_SEND[0]`.
The `IPC_IRQHandler` polls the registered Contiki-NG process on
`EVENTS_RECEIVE[0]`.

## Source Files

| File | Description |
|------|-------------|
| `arch/cpu/nrf/net/nrf-ipc.h` | Protocol definitions and shared memory layout |
| `arch/cpu/nrf/net/nrf-ipc.c` | IPC transport layer (channel setup, signalling, IRQ) |
| `arch/cpu/nrf/net/nrf-ipc-radio.c` | App core radio driver (`ipc_radio_driver`) |
| `arch/cpu/nrf/net/nrf-ipc-mac.c` | Net core IPC MAC driver (interrupt-driven frame forwarding) |
| `arch/cpu/nrf/os/dbg-arch.c` | Debug output; network core variant writes to IPC log buffer |
| `examples/platform-specific/nrf/ipc-radio-service/` | Network core radio service |

## Building and Flashing

The network core must run the `ipc-radio-service` firmware. Any standard
Contiki-NG application can then be built for the application core -- the
IPC radio driver is selected automatically.

```bash
# Build and flash the network core radio service
make -C examples/platform-specific/nrf/ipc-radio-service \
     TARGET=nrf BOARD=nrf5340/dk/network ipc-radio-service.upload

# Build and flash any application on the application core (e.g., rpl-udp)
make -C examples/rpl-udp \
     TARGET=nrf BOARD=nrf5340/dk/application udp-client.upload
```

Serial output is available on VCOM2 (`/dev/ttyACM2`) at 115200 baud.

### TrustZone Builds

The application core auto-selects its radio driver based on the build
mode (see `arch/cpu/nrf/nrf5340-application-def.h`):

| Build mode                              | `NETSTACK_RADIO`     |
|-----------------------------------------|----------------------|
| Plain (no TrustZone)                    | `ipc_radio_driver`   |
| TrustZone normal world (`TRUSTZONE=1`)  | `tz_radio_driver`    |
| TrustZone secure world                  | `nullradio_driver` (radio is reached via NSC calls) |

The `tz_radio_driver` keeps the IPC radio in the secure world and
exposes it to the normal world through Non-Secure Callable entry
points, enabling communication-policy enforcement at the TrustZone
boundary. Adding `TRUSTZONE=1` to any application build for
`nrf5340/dk/application` is enough -- the secure world is built,
merged with the normal world, and flashed automatically. See
`examples/platform-specific/nrf/trustzone/README.md` for details.

## Boot Sequence

1. The application core starts, clears shared memory, and initializes
   IPC channels.
2. It releases the network core from force-off via
   `nrf_reset_network_force_off()`.
3. The network core boots, initializes its IPC channels, sets
   `shm->net_ready = 1`, and enters its command loop.
4. The application core detects `net_ready`, sends `NRF_IPC_CMD_INIT`
   to initialize the radio, and proceeds with normal Contiki-NG
   startup (MAC, routing, application).

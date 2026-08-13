# IPC Radio Service

Radio service firmware for the nRF5340 **network core**. Provides
802.15.4 radio access to the application core via IPC shared memory.

This is a minimal Contiki-NG image whose sole purpose is to receive
commands from the application core, call the nRF 802.15.4 radio
driver, and return results.

## Building and Flashing

The network core must be flashed **before** the application core:

```bash
make -C examples/platform-specific/nrf/ipc-radio-service \
     TARGET=nrf BOARD=nrf5340/dk/network
make -C examples/platform-specific/nrf/ipc-radio-service \
     TARGET=nrf BOARD=nrf5340/dk/network ipc-radio-service.upload
```

## Optional: nrf_802154 radio with hardware auto-ACK

By default the service uses the raw radio driver
(`nrf-ieee-driver-arch.c`), which does not acknowledge frames in
hardware; the IPC MAC sends software ACKs instead, which can be too
slow for some peers.

Building with `NRF_802154=1` replaces the raw driver with Nordic's
nrf_802154 library, which performs address filtering and auto-ACK in
hardware within the 802.15.4 ACK window, and reports the transmit
verdict (ACK received or not) over IPC.

**Both cores must be built with the same setting.** The application
core needs `NRF_802154=1` too, so that CSMA trusts the transmit
verdict instead of polling for ACK frames (which a hardware-ACK net
core never forwards):

```bash
make -C examples/platform-specific/nrf/ipc-radio-service \
     TARGET=nrf BOARD=nrf5340/dk/network NRF_802154=1 ipc-radio-service.upload
make -C examples/rpl-udp TARGET=nrf BOARD=nrf5340/dk/application \
     NRF_802154=1 udp-server.upload
```

## Design

Frame reception is fully interrupt-driven via the IPC MAC driver:
the radio ISR triggers the radio driver process, which calls the
IPC MAC's input function to forward the frame to the app core via
shared memory. Between events, the CPU sleeps (WFI), minimizing
energy consumption and preventing bus stalls.

IPC commands from the app core are delivered via IPC interrupt.
The service process wakes only when there is actual work to do.

UARTE is disabled on the network core (`NRF_HAS_UARTE 0`). All
debug output is redirected to a shared memory ring buffer, which
the application core drains and prints with a `[NET]` prefix.

TSCH is not supported — the IPC latency is too high for TSCH slot
timing. Use CSMA (the default).

## Running RPL UDP over IPC

First, flash the network core radio service (see above). The
application core then automatically uses `ipc_radio_driver` to reach
the radio, so any standard Contiki-NG application builds and runs
unchanged. To flash the RPL UDP server on the application core:

```bash
make -C examples/rpl-udp TARGET=nrf BOARD=nrf5340/dk/application \
     udp-server.upload
```

On a second node (e.g., nRF52840 DK), flash the RPL UDP client:

```bash
make -C examples/rpl-udp TARGET=nrf BOARD=nrf52840/dk udp-client.upload
```

### TrustZone Mode

To run the same example with the radio driver behind the TrustZone
secure-world boundary, add `TRUSTZONE=1` to the application build:

```bash
make -C examples/rpl-udp TARGET=nrf BOARD=nrf5340/dk/application \
     TRUSTZONE=1 udp-server.upload
```

This recursively builds the bundled secure world, links the normal
world against its CMSE import library, merges both images, and flashes
the merged hex. See `../trustzone/README.md` for the full list of knobs
(custom secure world, etc.) and for the manual build flow.

## Related

- `arch/cpu/nrf/net/README.md` — Full IPC architecture documentation
- `arch/cpu/nrf/net/nrf-ipc.h` — IPC protocol definitions
- `arch/cpu/nrf/net/nrf-ipc-mac.c` — IPC MAC driver (net core)

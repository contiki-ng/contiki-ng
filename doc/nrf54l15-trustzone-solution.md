# nRF54L15 TrustZone Solution

This document describes the TrustZone solution that exists in this tree today.
It is a description of the current implementation, not only the port plan.

## What The Current Solution Is

The nRF54L15 TrustZone support is implemented as two separate Contiki-NG
firmware images:
- a secure image
- a non-secure image

Both images are built separately and merged into one HEX file for flashing.
The ordinary single-image non-TrustZone build still exists and is still the
normal way to build plain applications for `BOARD=nrf54l15/xiao`.

The current TrustZone solution is aimed at:
- booting a secure Contiki-NG image first
- handing off to a non-secure Contiki-NG image
- keeping the real 802.15.4 radio in secure world
- letting the non-secure world use the radio through a TrustZone proxy

Current hardware status:
- secure boot and secure-to-non-secure handoff work
- the minimal TrustZone example reaches non-secure hello-world
- the TrustZone `rpl-udp` server boots in non-secure world
- the non-secure DAG root emits periodic RPL DIOs on hardware
- secure and non-secure serial output are visually separated

Still not finished:
- end-to-end `udp-client` to `udp-server` packet exchange on two boards
- shell support in the TrustZone wrappers
- final SPU/MPC ownership policy for all peripherals and bus masters
- removal of all bring-up scaffolding

## Build Modes

Three build modes matter:

1. Plain non-TrustZone build
   - single image
   - direct platform drivers
   - same model as before the TrustZone port

2. TrustZone secure build
   - `TRUSTZONE_SECURE_BUILD=1`
   - builds the secure image

3. TrustZone non-secure build
   - `TRUSTZONE_SECURE_BUILD=0`
   - builds the paired non-secure image
   - not standalone; it depends on the secure image being flashed too

The split-image examples hide most of this and build both worlds plus the
merged HEX for you.

## Memory Layout

The current split uses:
- secure flash: `0x00000000` .. `0x0003ffff`
- non-secure flash: `0x00040000` .. `0x0015cfff`
- secure RAM: `0x20000000` .. `0x20017fff`
- non-secure RAM: `0x20018000` .. `0x2002efff`
- NSC veneer window: last `0x1000` bytes of the secure flash partition

The current definitions live in:
- `arch/cpu/nrf/nrf54l15/partition/region_defs.h`
- `arch/cpu/nrf/nrf54l15/tz-mdk/nrf54l15_xxaa_application_s.ld`
- `arch/cpu/nrf/nrf54l15/tz-mdk/nrf54l15_xxaa_application_ns.ld`

## World Split

### Secure World

The secure world currently owns:
- secure boot and TrustZone handoff
- SAU setup and the basic non-secure environment setup
- the NSC veneer region
- the real nRF54L15 802.15.4 radio driver
- the shared UART/console
- secure-side packet delivery into the non-secure stack

The main secure files are:
- `arch/cpu/arm/cortex-m/trustzone/tz-secure.c`
  - secure `platform_main_loop()`
  - TrustZone bring-up
  - jump to non-secure reset handler
- `arch/cpu/nrf/nrf54l15/tz-target-cfg.c`
  - nRF54L15-specific SAU and interrupt target-state setup
- `arch/cpu/arm/cortex-m/trustzone/tz-fault.c`
  - secure fault capture across reset
- `arch/cpu/arm/cortex-m/trustzone/tz-api.c`
  - NSC entry points
  - packet and radio calls crossing the world boundary
- `arch/cpu/arm/cortex-m/trustzone/tz-secure-mac.c`
  - secure MAC shim used in front of the non-secure stack
- `arch/cpu/nrf/dev/uarte-arch.c`
  - secure UART TX/RX path
- `arch/cpu/nrf/os/dbg-arch.c`
  - secure and non-secure serial formatting and color handling

The secure build currently links the real radio path. The key platform radio
piece is:
- `arch/cpu/nrf/nrf54l15/nrf-ieee-driver-nrf54l15.c`

### Non-Secure World

The non-secure world currently owns:
- applications
- the regular Contiki network stack
- RPL Lite, 6LoWPAN, CSMA, UDP, etc.
- the proxy radio driver used instead of the direct nRF54L15 radio driver

The main non-secure files are:
- `arch/cpu/arm/cortex-m/trustzone/normal/tz-normal.c`
  - initializes the TrustZone API
  - registers packet buffers and callbacks
  - polls secure world from non-secure world
- `arch/cpu/arm/cortex-m/trustzone/normal/tz-radio-driver.c`
  - non-secure radio proxy
  - forwards radio calls through the TrustZone API
- `arch/cpu/arm/cortex-m/trustzone/normal/module-macros.h`
  - selects `tz_radio_driver` in the non-secure TrustZone build

### What Crosses The Boundary

The TrustZone boundary currently carries:
- non-secure poll requests
- packet delivery from secure world into non-secure world
- radio operations from non-secure world into secure world
- serial output from non-secure world through secure UART

The boundary is defined in:
- `arch/cpu/arm/cortex-m/trustzone/tz-api.h`
- `arch/cpu/arm/cortex-m/trustzone/tz-api.c`

The non-secure side provides this callback structure:
- `request_poll`
- `process_packet_data`
- `packet_data`
- `packet_data_size`

That structure is registered by `tz_api_init()` during non-secure boot.

## Boot Flow

The current runtime flow is:

1. The secure image boots as a normal Contiki-NG image.
2. `platform_main_loop()` in `tz-secure.c` initializes TrustZone support.
3. Secure code configures SAU, the NSC window, and the non-secure CPU state.
4. Secure code routes the required interrupts to non-secure world.
5. Secure code jumps to the non-secure reset handler.
6. The non-secure image boots as a normal Contiki-NG image.
7. `platform_main_loop()` in `tz-normal.c` calls `tz_api_init()`.
8. The non-secure image registers packet buffers and callback functions.
9. The secure world and non-secure world then cooperate through:
   - `tz_api_poll()`
   - `tz_api_process_packet_data()`
   - `tz_api_radio_*()`

## Radio Path

The current radio design is:
- secure world owns the real radio and radio-adjacent platform pieces
- non-secure world sees `NETSTACK_RADIO == tz_radio_driver`

Transmit path:
- non-secure stack calls `tz_radio_driver`
- `tz_radio_driver` calls `tz_api_radio_*()`
- secure world executes the real radio operation

Receive path:
- secure radio receives the frame
- `tz_secure_mac_driver` hands the frame to the TrustZone API
- secure code copies packet data into the validated non-secure buffer
- non-secure `process_packet_data()` calls `NETSTACK_MAC.input()`

This is the intended long-term ownership model for the nRF54L15 port as well.

## Logging And Console Behavior

The current serial model is:
- secure world owns the UART
- non-secure prints are forwarded through `tz_api_println()`

Current formatting:
- secure lines are printed directly, without a prefix
- non-secure lines use the `n>` prefix
- secure lines are colored cyan
- non-secure lines are colored green

The relevant files are:
- `arch/cpu/nrf/os/dbg-arch.c`
- `arch/cpu/arm/cortex-m/trustzone/tz-api.c`

Current fault diagnostics:
- previous hard-fault info is dumped early at boot if available
- secure fault status is preserved across reset

The relevant files are:
- `arch/cpu/nrf/arm/hardfault-handler.c`
- `arch/cpu/arm/cortex-m/trustzone/tz-fault.c`
- `arch/platform/nrf/platform.c`

## Examples

### Minimal TrustZone Example

Path:
- `examples/platform-specific/nrf/trustzone`

Purpose:
- prove basic secure/non-secure split
- keep the non-secure path intentionally small

Current role:
- boot and handoff smoke test
- minimal non-secure hello-world path

### TrustZone RPL-UDP Wrapper

Path:
- `examples/platform-specific/nrf/trustzone/rpl-udp`

Purpose:
- run a real non-secure networking application under TrustZone

Current role:
- main validation target for timers, radio proxying, and network bring-up

## Build And Merge Wiring

The main build selection lives in:
- `arch/cpu/nrf/nrf54l15/Makefile.nrf54l15`
- `arch/cpu/arm/cortex-m/Makefile.cortex-m`

The example-level split and HEX merge wiring lives in:
- `examples/platform-specific/nrf/trustzone/Makefile`
- `examples/platform-specific/nrf/trustzone/rpl-udp/Makefile`

## Bring-Up Scaffolding Still Present

The current solution still contains bring-up scaffolding that should be treated
as temporary:
- `arch/cpu/nrf/nrf54l15/tz-spu.c`
  - placeholder SPU layer for the generic TrustZone runtime
- `arch/cpu/nrf/nrf54l15/spu.h`
  - placeholder SPU interface used by the current port
- `arch/cpu/nrf/nrf54l15/tz-minimal-platform.c`
  - minimal non-secure platform path used during bring-up
- `TZ_DEBUG_MASK_NS_IRQS`
  - debug configuration used while fixing interrupt routing
- `TZ_RPL_UDP_DEBUG`
  - optional extra logging for timer/IRQ bring-up

These pieces are useful for stabilization, but they are not the final shape of
the platform.

## Current Limitations

The current TrustZone solution should be understood as "working bring-up plus
working secure-owned radio proxy", not "finished production split".

Known limitations:
- full SPU/MPC ownership is not implemented
- shell input is not enabled in the TrustZone wrappers
- the UART/console path has been stabilized, but it is still an active debug
  area
- over-the-air two-board `udp-client` / `udp-server` validation is still
  pending
- OpenOCD flashing on the XIAO is less reliable with `verify_image` than with
  `nrf54l-load` alone

## Where To Start Reading

If you need to understand the current solution quickly, read in this order:

1. `examples/platform-specific/nrf/trustzone/README.md`
2. `examples/platform-specific/nrf/trustzone/rpl-udp/README.md`
3. `arch/cpu/arm/cortex-m/trustzone/tz-secure.c`
4. `arch/cpu/arm/cortex-m/trustzone/normal/tz-normal.c`
5. `arch/cpu/arm/cortex-m/trustzone/tz-api.c`
6. `arch/cpu/arm/cortex-m/trustzone/normal/tz-radio-driver.c`
7. `arch/cpu/arm/cortex-m/trustzone/tz-secure-mac.c`
8. `arch/cpu/nrf/nrf54l15/tz-target-cfg.c`
9. `arch/cpu/nrf/nrf54l15/Makefile.nrf54l15`

## Related Documents

The current implementation grew out of these two documents:
- `doc/nrf54l15-trustzone-port.md`
- `doc/nrf54l15-trustzone-radio-plan.md`

Use this document for the current layout.
Use those documents for the port history, rationale, and remaining work.

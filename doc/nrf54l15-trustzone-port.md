# nRF54L15 TrustZone Port Plan

This document turns the high-level nRF54L15 port TODO into an implementation plan for the first TrustZone-enabled bring-up on Contiki-NG.

## Scope

The goal of phase 1 is to make the existing Contiki TrustZone framework build and boot on `BOARD=nrf54l15/xiao` with a split secure/non-secure image layout.

Phase 1 is intentionally narrow:
- secure world boots first
- secure world configures a minimal secure/non-secure memory split
- secure world exposes the NSC veneer window for `tz_api_init()` and `tz_api_poll()`
- secure world jumps to the non-secure reset vector
- the normal-world example reaches a minimal hello-world path
- the secure image owns the real nRF54L15 radio while the normal-world image uses a TrustZone radio proxy

Phase 1 does not claim full nRF54L15 security attribution coverage. In particular, it does not yet provide a complete MPC/SPU policy for all peripherals and bus masters.

## Why nRF54L15 Needs a Different Port Than nRF5340

The nRF5340 TrustZone code is a useful structural reference, but nRF54L15 differs in the security hardware:
- nRF5340 largely uses a single SPU-centric memory attribution model in the existing Contiki code
- nRF54L15 exposes multiple SPU instances (`SPU00`, `SPU10`, `SPU20`, `SPU30`) plus `MPC00`
- the existing generic `arch/cpu/nrf/dev/spu.c` is written around the older flash-region / sram-region model and should not be treated as a correct direct port for nRF54L15 memory attribution

For that reason, the first implementation uses:
- explicit secure and non-secure linker scripts
- explicit Contiki partition headers for nRF54L15
- a minimal `tz-target-cfg` that configures the SAU and non-secure handoff
- a conservative placeholder `tz-spu` layer so the generic TrustZone runtime can build

That gives a truthful phase-1 port and keeps the remaining hardware-specific work visible.

## Phase 1 Memory Layout

The current single-image linker script for nRF54L15 uses:
- flash: `0x00000000` .. `0x0015cfff`
- ram: `0x20000000` .. `0x2002efff`

Phase 1 splits this into:
- secure flash: `0x00000000` .. `0x0003ffff`
- non-secure flash: `0x00040000` .. `0x0015cfff`
- secure ram: `0x20000000` .. `0x20017fff`
- non-secure ram: `0x20018000` .. `0x2002efff`
- NSC veneer window: last `0x1000` bytes of the secure flash partition

Rationale:
- the split is simple and aligned to 4 KiB boundaries
- the secure example is small enough for a 256 KiB secure partition
- the veneer window is fixed and large enough for the current CMSE gateway set
- the non-secure image still gets most of the available flash and ram

## Implementation Steps

### 1. Build-System Split
- add secure and non-secure linker scripts for nRF54L15
- add `arch/cpu/nrf/nrf54l15/partition/flash_layout.h`
- add `arch/cpu/nrf/nrf54l15/partition/region_defs.h`
- make `arch/cpu/nrf/nrf54l15/Makefile.nrf54l15` select those files when `TRUSTZONE_SECURE_BUILD` is `1` or `0`

### 2. Secure Runtime Bring-Up
- add `tz-target-cfg.h` and `tz-target-cfg.c` for nRF54L15
- enable secure fault, memfault, busfault, and usage fault handling
- configure SAU regions for:
  - non-secure flash
  - non-secure ram
  - the secure veneer window as NSC
- configure `SCB_NS->VTOR`, `MSP_NS`, `PSP_NS`, and `CONTROL_NS`
- transfer control to the non-secure reset handler

### 3. SPU/MPC Handling Strategy
- do not reuse the old nRF53 memory-region SPU implementation as if it were correct on nRF54L15
- provide a minimal `tz-spu.c` for the phase-1 build so the generic TrustZone runtime links cleanly
- leave full SPU/MPC policy programming as explicit phase-2 work

### 4. Example Retargeting
- retarget `examples/platform-specific/nrf/trustzone` to `BOARD=nrf54l15/xiao`
- keep the existing two-image build and merge flow
- keep the secure-fault demonstration in the normal-world example

## Validation Plan

### Compile-time checks
- secure world builds with `TRUSTZONE_SECURE_BUILD=1`
- non-secure world builds with `TRUSTZONE_SECURE_BUILD=0`
- merged hex is produced from both images
- compile test entry uses `BOARD=nrf54l15/xiao`

### Runtime checks
- secure world logs TrustZone initialization
- secure world accepts `tz_api_init()` from non-secure world
- non-secure world prints its startup banner
- secure periodic timer event still runs after handoff
- long-press or explicit test path can trigger the secure-fault reboot path

### Current runtime status on hardware
- the secure world reaches the non-secure handoff on `nrf54l15/xiao`
- the secure image now exports a working NSC window; `__sg_start` and `__sg_end` resolve to the actual veneer range
- the normal-world example now boots far enough to print `n> non-secure hello world` through `tz_api_println()`
- the secure build now links the real `nrf_ieee_driver_nrf54l15`
- the non-secure build now links `tz_radio_driver`
- the current hardware log also shows `n> non-secure radio ready on channel 26`, which proves non-secure code can query and turn on the secure-owned radio
- the non-secure `rpl-udp` server now runs on hardware, its 1 Hz heartbeat is ticking, and the DAG root emits multicast DIOs
- the timer/IRQ handoff issue was traced to `#ifdef ...IRQn` guards in `tz-target-cfg.c`; those guards compiled out the actual `ITNS` routing on nRF54L15 because the IRQ names are enum constants, not preprocessor macros
- this path still depends on bring-up scaffolding:
  - extra timer and IRQ diagnostics in the non-secure `udp-server` build
  - a secure-side IRQ route log in `tz-secure.c`
- the full normal-world platform is still incomplete; over-the-air packet validation, cleanup of the debug diagnostics, and final interrupt/peripheral ownership are not finished
- the radio follow-up is documented in `doc/nrf54l15-trustzone-radio-plan.md`

### Manual feature tests to add next
The richer `contiki-ng-tz` tree already contains useful manual validation patterns that should be copied or adapted after phase 1:
- `memfault` command
- `ns-perf` and `s-perf` cross-world latency tests
- secure / normal shell commands for memory read-write inspection
- policy-monitor integration in secure world

## Explicit TODO After Phase 1

The following work remains after the first split-image bring-up:
- remove the phase-1 hello-world shims once the final interrupt attribution model is in place:
  - `TZ_DEBUG_MASK_NS_IRQS`
  - the temporary `GRTC_2_IRQHandler` stub
  - the minimal non-secure platform build path
- replace the phase-1 placeholder `tz-spu` layer with a real nRF54L15 SPU/MPC implementation
- define the final peripheral ownership matrix for GPIO, GRTC, UARTE, watchdog, and radio
- extend the local TrustZone API further if serial forwarding or shell support is needed; the current port only carries the radio and packet handoff subset
- validate actual over-the-air radio TX/RX through the new proxy path
- decide whether the radio should remain fully secure on nRF54L15 or whether a different split is needed after the forwarding driver is proven
- add TrustZone-specific runtime tests under `tests/`, not only compile coverage
- port the shell-driven validation helpers from the `trustzone/common` support in `contiki-ng-tz`
- document any required secure/non-secure pin and interrupt targeting rules for the XIAO board

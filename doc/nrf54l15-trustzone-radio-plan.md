# nRF54L15 TrustZone Radio Plan

This document captures the current nRF54L15 TrustZone radio state and the implementation plan for moving from the current non-secure hello-world bring-up to a working 802.15.4 radio path.

## Current State

What works on hardware today:
- the secure image boots first
- the secure image configures SAU/MPC state and a working NSC veneer window
- the secure image jumps into the non-secure reset handler
- the non-secure image reaches a minimal Contiki process and prints `n> non-secure hello world` via `tz_api_println()`
- the secure image now owns the real nRF54L15 802.15.4 driver
- the non-secure image now links `tz_radio_driver`
- the non-secure example can query the secure-owned radio and print `n> non-secure radio ready on channel 26`
- a TrustZone `rpl-udp` wrapper now builds both `udp-client` and `udp-server` as non-secure images
- the `udp-server` variant boots on hardware, reaches `RPL Lite + sicslowpan + CSMA`, and creates the DAG root

What is intentionally still disabled in the current hello-world path:
- the non-secure image does not own UARTE
- the normal-world build still uses `nullmac` and `nullnet`
- the current hello-world depends on debug bring-up shims:
  - `TZ_MINIMAL_NONSECURE_PLATFORM`
  - `TZ_DEBUG_MASK_NS_IRQS`
  - a temporary `GRTC_2_IRQHandler` sink in the non-secure image

What this means in practice:
- the TrustZone handoff is now proven
- the secure-owned radio proxy model is now in place
- basic radio init/on from non-secure code is proven
- a real IPv6/RPL non-secure image now boots under TrustZone
- over-the-air TX/RX is not proven yet

## Target Architecture

The target design for nRF54L15 should match the TrustZone model already used in `contiki-ng-tz`:
- secure world owns the real 802.15.4 radio driver and the radio-adjacent peripherals
- non-secure world uses a proxy radio driver (`tz_radio_driver`)
- packet TX/RX crosses worlds through the TrustZone API using validated non-secure buffers
- secure world remains the policy/ownership boundary for radio, clocks, and interrupts

That design is preferable to giving the non-secure image direct access to `nrf_ieee_driver_nrf54l15` because:
- the current TrustZone split already assumes secure-side ownership of critical IRQs and clocks
- the radio library on nRF54L15 touches multiple timing and interrupt sources, not only `RADIO`
- the secure-owned proxy model is already implemented and debugged structurally in `contiki-ng-tz`

## Delta Between This Tree and the Reference Model

Reference pieces that already exist in `contiki-ng-tz`:
- expanded TrustZone API in `arch/cpu/arm/cortex-m/trustzone/tz-api.h` and `tz-api.c`
- non-secure proxy radio driver in `arch/cpu/arm/cortex-m/trustzone/normal/tz-radio-driver.c`
- secure-side MAC shim in `arch/cpu/arm/cortex-m/trustzone/tz-secure-mac.c`
- normal-world module wiring in `arch/cpu/arm/cortex-m/trustzone/normal/module-macros.h`
- secure-world module wiring in `arch/cpu/arm/cortex-m/trustzone/module-macros.h`
- build wiring that includes `trustzone/common` in `arch/cpu/arm/cortex-m/Makefile.cortex-m`

What is still missing in this tree:
- the local `tz-api` only implements the radio and packet-forwarding subset, not the full `trustzone/common` serial/shell support from `contiki-ng-tz`
- `Makefile.cortex-m` still does not pull in the broader `trustzone/common` support used by the reference tree
- the current non-secure example proves radio init/on only; it does not yet prove real packet TX/RX

## Radio Ownership Assumption

The first working radio port should keep the radio fully secure.

Secure side should own at least:
- `RADIO_0_IRQn`
- `RADIO_1_IRQn`
- `EGU10_IRQn`
- `GRTC` IRQs used by the secure clock/rtimer path
- `CLOCK_POWER_IRQn`
- `TIMER10` resources used by timestamping
- `DPPIC10` and `DPPIC20` resources touched by the `nrf_802154` stack
- HFXO/PLL start-up and LFCLK policy used by `nrf_802154_platform_clock.c`

This matches the current nRF54L15 radio wrapper better than a split-ownership approach:
- `nrf_802154_platform_irq.c` dispatches `RADIO_0`, `RADIO_1`, and `EGU10`
- `nrf_802154_platform_clock.c` owns HFXO/PLL bring-up
- `nrf_802154_platform_sl_lptimer.c` depends on GRTC
- `nrf_802154_platform_timestamper.c` uses `TIMER10` plus local DPPI wiring

## Implementation Plan

### Phase 1: Port The TrustZone Radio Plumbing

Status:
- done

Goal:
- make the local TrustZone runtime structurally match the reference proxy-radio design

Changes:
- extend the local `tz-api.h` and `tz-api.c` with the radio and packet-forwarding subset needed for the proxy driver
- add `arch/cpu/arm/cortex-m/trustzone/normal/tz-radio-driver.c`
- add `arch/cpu/arm/cortex-m/trustzone/tz-secure-mac.c`
- update `arch/cpu/arm/cortex-m/trustzone/normal/module-macros.h` to define `NETSTACK_CONF_RADIO tz_radio_driver`
- update `arch/cpu/arm/cortex-m/trustzone/module-macros.h` to define:
  - `NETSTACK_CONF_MAC tz_secure_mac_driver`

Result:
- the non-secure build no longer uses the direct nRF54L15 radio driver
- packet buffers are registered through `tz_api_init()`
- secure RX is forwarded through `tz_secure_mac_driver` into the non-secure callback path

### Phase 2: Make The Secure World Own The Real Radio

Status:
- done for bring-up

Goal:
- move from hello-world to a real secure-owned 802.15.4 driver

Changes:
- keep `nrf-ieee-driver-nrf54l15.c` and the `nrf_802154_platform_*` files in the secure build
- keep those files excluded from the non-secure build
- change the secure TrustZone example from `NETSTACK_CONF_RADIO nullradio_driver` to the real nRF54L15 radio driver
- keep the normal-world example on `tz_radio_driver`
- keep the secure-side MAC shim in front of normal-world packet delivery

Result:
- secure world owns the real nRF54L15 radio driver
- non-secure world uses `tz_radio_driver`
- non-secure code can turn on the secure-owned radio and query its channel
- over-the-air TX/RX still needs explicit validation

### Phase 3: First Functional Radio Test With Minimal Networking

Goal:
- prove the proxy path before restoring the full non-secure platform

Recommended configuration:
- non-secure side:
  - `MAKE_MAC = MAKE_MAC_NULLMAC`
  - `MAKE_NET = MAKE_NET_NULLNET`
  - `NETSTACK_CONF_RADIO tz_radio_driver`
- secure side:
  - real nRF54L15 radio driver

Test shape:
- use a simple NullNet sender/receiver pair or a direct `NETSTACK_RADIO.send()` smoke test
- send a short payload from the non-secure world
- confirm secure radio TX path works
- confirm a received frame is delivered back into the non-secure world through `process_packet_data()`

Exit criteria:
- non-secure world can call `NETSTACK_RADIO.send()`
- a frame sent over the air is visible on a second board or sniffer
- a received frame reaches non-secure `NETSTACK_MAC.input()`

### Phase 4: Remove Bring-Up Shims Incrementally

Goal:
- replace the debug-only hello-world scaffolding with the real platform split

Order:
1. remove `TZ_DEBUG_MASK_NS_IRQS`
2. remove the temporary `GRTC_2_IRQHandler` sink in the non-secure image
3. restore the normal non-secure platform path where safe
4. restore non-secure timers only after final GRTC ownership is explicit
5. decide whether non-secure UARTE stays proxied or gets reassigned later

Rule:
- do not remove these shims all at once
- each removal must be accompanied by a hardware check that the radio proxy path still works

### Phase 5: Move Beyond NullNet

Goal:
- prove that the proxy radio path is good enough for the regular Contiki stack

Suggested progression:
1. `nullmac` + `nullnet`
2. `nullmac` + packet receive path stress
3. `csma` + `nullnet`
4. IPv6/6LoWPAN once the above is stable

This order matters because it isolates:
- radio ownership bugs
- packet copy / callback bugs
- MAC timing bugs
- full-stack network bugs

## Validation Matrix

The radio work should be considered done only when all of these pass on hardware.

### Milestone A: Proxy API
- `tz_api_init()` succeeds with packet and serial buffers registered
- non-secure build links against `tz_radio_driver`
- secure build links against the real nRF54L15 radio

### Milestone B: TX
- non-secure `NETSTACK_RADIO.send()` succeeds through `tz_api_radio_send()`
- secure world transmits an 802.15.4 frame over the air
- no reboot or secure fault occurs during repeated TX

### Milestone C: RX
- secure radio ISR path receives a frame
- secure MAC shim copies payload to non-secure buffer
- non-secure `NETSTACK_MAC.input()` runs
- repeated RX does not wedge GRTC or the packet buffer path

### Milestone D: Stability
- repeated TX/RX survives for minutes, not only one packet
- secure timer output still runs
- no `SecureFault INVEP`
- no return to the temporary hello-world-only boot state

## Risks To Handle Explicitly

- NSC/veneer regressions:
  - the first successful hello-world depended on fixing the secure linker script so `__sg_end` is correct
  - any merge of the richer `tz-api` must preserve that working veneer region

- IRQ ownership regressions:
  - `GRTC_2` already proved that a wrongly targeted IRQ can break non-secure bring-up before `main()`
  - radio IRQ ownership must be explicit, not inferred

- clock ownership regressions:
  - the nRF54L15 radio wrapper currently assumes secure-side HFXO/PLL control
  - moving any of that back to non-secure too early will likely reintroduce early faults

- over-restoring the non-secure platform:
  - the first goal is a working proxy radio path
  - full non-secure platform parity can come later

## Immediate Next Coding Step

The next implementation step should be:
- run the new TrustZone `rpl-udp` wrapper on two boards
- validate TX first from non-secure `udp-client` to non-secure `udp-server`
- validate RX delivery back into the non-secure callback path under `CSMA + sicslowpan + RPL Lite`
- then investigate whether the remaining bring-up shims can be reduced safely

That is the shortest path from the current proxy bring-up to a proven radio path.

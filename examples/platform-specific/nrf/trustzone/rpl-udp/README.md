# TrustZone RPL-UDP Example

This example wraps the existing `examples/rpl-udp` application in the
nRF54L15 TrustZone split-image build.

For the current TrustZone layout and file map, see:
- `doc/nrf54l15-trustzone-solution.md`

The secure image owns the real radio and the non-secure image runs either:
- `udp-client`
- `udp-server`

Build the default non-secure client:

```sh
make
```

Build the non-secure server instead:

```sh
make APP=udp-server
```

Flash the merged image:

```sh
make upload APP=udp-client
```

Open the serial console:

```sh
make login PORT=/dev/<PORT>
```

Current limitations:
- the secure-owned radio proxy is in place, and the `udp-server` DAG root now emits periodic RPL DIOs on hardware
- over-the-air client/server packet exchange is still not validated end-to-end
- extra timer/IRQ diagnostics from the TrustZone bring-up are available behind `TZ_RPL_UDP_DEBUG=1`, but are disabled by default because they make the serial log noisy
- `BUILD_WITH_SHELL` is intentionally not enabled for this wrapper

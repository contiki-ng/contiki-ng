# hello-vpr

Smallest Contiki-NG on the nRF54L15 FLPR (RV32E coprocessor).

One process, etimer-driven LED blink (gpio2 pin 0, the XIAO nRF54L15 user LED,
1 Hz), tick counter written to a known shared SRAM address (`0x2003F000`) so
the M33 can read it.

Builds for `TARGET=nrf-vpr`. The resulting `.bin` is the FLPR firmware blob
the M33-side `flpr-host` app (in the sibling directory) embeds in its image.

    gmake TARGET=nrf-vpr WERROR=0

Output: `build/nrf-vpr/hello-vpr.bin` (~8 KB).

Cannot be flashed standalone — see the sibling `flpr-host` example for the M33
companion that launches it via the SPU + VPR boot dance.

See `doc/platforms/nrf-vpr.md` for the full port documentation.

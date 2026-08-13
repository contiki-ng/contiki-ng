# nRF Radio Test

This example is a shell-controlled MAC-level radio test for nRF boards. It uses
`nullnet` and explicit unicast MAC transmissions with link-layer ACK requested.

It is intended for bring-up and debugging of the `nrf_802154` integration,
especially when you need to separate:

- payload loss
- ACK timing / ACK validation failures
- retry behavior under different power, length, and channel settings

## Manual use

Build and flash on each board:

```sh
gmake -C examples/platform-specific/nrf/radio-test TARGET=nrf BOARD=nrf54l15/dk radio-test.flash -j4
gmake -C examples/platform-specific/nrf/radio-test TARGET=nrf BOARD=nrf54l15/xiao radio-test.flash -j4
```

Useful shell commands:

```text
radio-test status
radio-test status-brief
radio-test target <mac>
radio-test clear-target
radio-test start
radio-test stop
radio-test once
radio-test interval <ms>
radio-test len <bytes>
radio-test txmax <n>
radio-test channel [n]
radio-test power [dbm]
radio-test verbose <0|1>
radio-test reset
```

`status-brief` prints a single `RTSTAT ...` line intended for scripts.

## Automated runner

The helper script `run-radio-test.py` drives two serial consoles, configures
both nodes, runs the requested test matrix in both directions, and prints a
summary.

Constraints:

- both boards must already run the `radio-test` firmware
- no other program should hold the serial ports open
- the script uses only Python standard library modules

Auto-discover exactly two radio-test nodes:

```sh
examples/platform-specific/nrf/radio-test/run-radio-test.py
```

Use explicit ports:

```sh
examples/platform-specific/nrf/radio-test/run-radio-test.py \
  --ports /dev/tty.usbmodem0010577532811 /dev/tty.usbmodemC4073FDF3
```

Run a small sweep and save JSON results:

```sh
examples/platform-specific/nrf/radio-test/run-radio-test.py \
  --count 20 \
  --powers 0,8 \
  --channels 26,20 \
  --lengths 20,50,100 \
  --txmax-values 1,3 \
  --json-out /tmp/nrf54l15-radio-test.json
```

Summary classification:

- `pass`: sender saw no `NOACK` / `ERR`
- `ack-path`: sender saw `NOACK`, receiver still reported payload RX
- `payload-loss`: sender saw `NOACK`, receiver reported no payload RX
- `other`: anything else

# IPv4 uplink for the nRF: ENC28J60 + IP64

How to give an nRF board an Ethernet uplink with an ENC28J60 module and turn
it into a self-contained border router: RPL root on the 802.15.4 side, NAT64
and DNS64 on the IPv4 side, no Linux host in the data path.

Verified end to end on an nRF54L15 DK with a HanRun HR911105A module, with a
Seeed XIAO nRF54L15 as a mesh node.

## What you need

* An nRF board with a free SPI instance and four spare GPIOs.
* An ENC28J60 module. These are 3.1-3.6 V parts; some vendor listings say 5 V
  in their pin tables, which is wrong.
* Four jumpers plus power and ground.

Nothing needs to be soldered, and the module's `INT`, `RESET`, `WOL` and
`CLKOUT` pins stay unconnected: the driver polls on a 1-tick etimer and
soft-resets over SPI.

## Set the GPIO voltage first

**The nRF54L15 DK ships with VDD at 1.8 V.** An ENC28J60 will not run at that
voltage. Raise it before connecting anything, in nRF Connect for Desktop ->
Board Configurator: select the DK, set VDD to **3300 mV**, and write the
config. It persists across power cycles.

Do not use 3000 mV, the value most guides show as their example -- it is below
the ENC28J60's 3.1 V minimum.

Powering the module from a separate 3.3 V supply while the DK is still at
1.8 V is worse than not working: the module's `SO` output would drive 3.3 V
into a 1.8 V nRF pin.

## Wiring, nRF54L15 DK

Header **PORT P1**, the middle of the three GPIO headers. It is a dual-row
header; read the printed pin numbers rather than counting positions.

| Module | DK       | Signal                  |
|--------|----------|-------------------------|
| `SCK`  | `P1.11`  | SPI clock               |
| `SI`   | `P1.06`  | MOSI, DK to module      |
| `SO`   | `P1.07`  | MISO, module to DK      |
| `CS`   | `P1.12`  | Chip select, active low |
| `VCC`  | `VDDIO`  | 3.3 V, see above        |
| `GND`  | `GND`    |                         |

Module pin names are from the module's point of view, which is why `SI` takes
the DK's MOSI and `SO` feeds its MISO. Swapping those two is the most common
miswire.

### Why these pins

PORT P1 only brings out P1.04 to P1.14 -- the DK silkscreen puts the others in
parentheses, meaning they need a board modification. Of the eleven that remain,
the console UART (P1.04/05), three buttons (P1.08/09/13) and two LEDs
(P1.10/14) already claim seven, leaving exactly four pins for four signals.

That is also why chip select is on P1.12 rather than P1.10, where Nordic's own
devicetree places this SPI's CS: P1.10 is an LED on this board.

SPIM22 is the instance behind the expansion header. SPIM00 drives the on-board
flash, and SPIM20 shares a SERIAL slot with the console UART, so enabling it
fails at link time with a duplicate `SERIAL20_IRQHandler`.

## Configuration

`WITH_IP64=1` pulls in `os/services/ip64`. The board or example must then
supply three things:

1. `NRF_WITH_SPI=1` and `NRF_SPI_INSTANCES`, to build the SPI driver.
2. `MODULES += $(CONTIKI_NG_DRIVERS_ETHERNET_DIR)/enc28j60`.
3. `CFLAGS += -DUIP_FALLBACK_INTERFACE=ip64_uip_fallback_interface`.

plus an `ip64-conf.h` on the include path selecting the Ethernet driver, and
`ENC28J60_CONF_USE_SPI_HAL 1` with the `ETH_SPI_*` pin defines.
`examples/platform-specific/nrf/ip64-router` has all of this.

The ENC28J60 arch layer used here is
`arch/dev/ethernet/enc28j60/enc28j60-arch-spi-hal.c`, which sits on the SPI HAL
rather than on a CPU's registers, so it works on any platform implementing
`os/dev/spi.h` -- a board only supplies pin defines.

## Bringing it up

Work outwards, so each step isolates one layer.

### 1. The SPI bus

    cd examples/platform-specific/nrf/spi-flash
    make TARGET=nrf BOARD=nrf54l15/dk spi-flash.upload

Talks to the DK's on-board flash on SPIM00, so it proves the SPI driver before
any external wiring exists. Expect a JEDEC ID of `c2 28 17`.

### 2. The ENC28J60

    cd examples/platform-specific/nrf/enc28j60-test
    make TARGET=nrf BOARD=nrf54l15/dk enc28j60-test.upload

Probes the chip before handing it to the driver, because `enc28j60_init()`
waits for `ESTAT.CLKRDY` in a loop with no timeout -- on a miswired board it
hangs silently. A good run:

    ESTAT:    01  OK (CLKRDY set)
    EREVID:   06  OK (silicon rev B7)
    reg r/w:  5a  OK
    MAC r/w:  02:de:ad:be:ef:01  OK
    ENC28J60 OK

`ff` everywhere means nothing is driving MISO: check MISO, CS, and that `SI`
and `SO` are not swapped. `00` everywhere means MISO is stuck low, usually
power or ground. Anything else points at SCK or too high a bit rate.

With a cable in a live switch it then prints received frames.

### 3. The router

    cd examples/platform-specific/nrf/ip64-router
    make TARGET=nrf BOARD=nrf54l15/dk ip64-router.upload

Becomes the RPL root and brings up IP64. It reports the address DHCP gives it:

    IPv4 address acquired:
      address   192.168.101.103
      netmask   255.255.255.0
      gateway   192.168.101.1

At that point the board answers pings from your LAN.

The example can also prove the translator without a second board. Define
`NAT64_TEST_ADDR` to an IPv4 host running `nc -u -l 7777` and the router sends
UDP through its own NAT64 prefix: a locally generated packet to an off-link,
unroutable destination takes the same uIP fallback path
(`tcpip.c: output_fallback`) that a forwarded one does.

### 4. A mesh node

    cd examples/platform-specific/nrf/nat64-node
    make TARGET=nrf BOARD=nrf54l15/xiao nat64-node.flash

Joins the RPL network and sends UDP to an IPv4 host, which exercises what the
router's self-test cannot: the forwarding hop, and a per-source entry in
ip64's address map. With both running, a listener sees one IPv4 source address
and two distinct mapped ports.

It also resolves a hostname through DNS64, pointing the resolver at a public
IPv4 DNS server via the NAT64 prefix:

    DNS64 OK: leshan.eclipseprojects.io -> 64:ff9b::23.97.187.154

Use this rather than hardcoding IPv4 literals in NAT64 form; those go stale.

## Things that will cost you time

**Reading the DK console needs DTR.** P1.06 and P1.07 double as the on-board
debugger's UART RTS/CTS. Once the ENC28J60 is wired to them, a terminal that
does not assert DTR reads zero bytes at every baud rate -- indistinguishable
from dead firmware. Open both VCOM ports, read the second at 115200, and assert
DTR and RTS. If output is still missing, halt the core over J-Link and resolve
the PC: `platform_idle` means the firmware is fine and the capture path is not.

**Firmware that prints once at boot loses it.** The banner is emitted while the
debugger is still resetting the target. Print periodically instead, or pulse
reset only after the serial port is open.

**Keep the bus slow to start.** 4 MHz is reliable on jumper wires. At 8 MHz a
`ESTAT` of `02` instead of `01` -- a one-bit shift -- means MISO is being
sampled too early; that is signal integrity, not a driver fault. Short leads
with ground returns, or moving MOSI/MISO off the debugger's pins with
`BUTTON_HAL_CONF_ENABLED 0`, raise the ceiling. Throughput is bounded by the
driver's 1-tick receive poll anyway.

**RPL join takes about a minute.** A node printing "waiting to join" for 40
seconds has not failed.

**An ENC28J60 draws well over 100 mA with the link up.** If the board browns
out when the cable goes in, power the module separately and tie the grounds --
with the DK at 3.3 V the logic levels still match.

## Status of os/services/ip64

`ip64` had no runtime test and no functional commits for years, and the Zoul
Orion was the only board in the tree that could run it. Everything above was
brought up against that code as-is; the translator, the address map, the DHCP
client and DNS64 all work. Treat unexplained behaviour as plausibly a latent
bug in the module rather than in your wiring, and note that the examples here
are the only runtime coverage it has.

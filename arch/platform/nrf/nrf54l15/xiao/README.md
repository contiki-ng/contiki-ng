# Seeed Studio XIAO nRF54L15 - Contiki-NG Port

## Board Overview

The Seeed Studio XIAO nRF54L15 is a compact development board featuring the Nordic nRF54L15 SoC with Bluetooth LE 5.4 and 802.15.4 support. This port brings Contiki-NG support to this platform.

**Board Specifications:**
- **MCU:** nRF54L15 (ARM Cortex-M33, 128 MHz)
- **Flash:** 1536 KB
- **RAM:** 256 KB
- **Wireless:** Bluetooth LE 5.4, 802.15.4 (2.4 GHz)
- **Debugger:** Onboard CMSIS-DAP (SAMD11)
- **Form Factor:** XIAO (21×17.8mm)
- **Vendor:** Seeed Studio

## Current Port Status

### ✅ Working Features:
- Build system integration
- GRTC-based clock and rtimer (using GRTC_0)
- GPIO HAL with nrfx v3.x API
- UART console (115200 baud) on UART20 (TX=P1.9, RX=P1.8)
- User LED on P2.0
- User button on P0.0
- RF front-end switch control (PWR=P2.3, SEL=P2.5)
- OpenOCD flashing via CMSIS-DAP
- 802.15.4 radio via Nordic `nrf_802154` driver (CSMA, ACKs)
- IPv6 networking stack (RPL + UDP examples)

### ⏳ Pending Features:
- Low-power modes
- Watchdog integration
- Temperature sensor

## Quick Start

### Prerequisites

1. **ARM GCC Toolchain:**
   ```bash
   # macOS
   brew install gcc-arm-embedded

   # Ubuntu/Debian
   sudo apt-get install gcc-arm-none-eabi
   ```

2. **OpenOCD (0.12.0 or newer):**
   - A stock OpenOCD release works — no nRF54L15-specific OpenOCD flash driver
     is needed. The board config (`support/openocd.cfg`) write-enables RRAM and
     programs it with `load_image` (direct memory writes), so it never invokes
     an OpenOCD flash-bank/`program` command.
   - macOS: `brew install openocd`; Linux: `apt-get install openocd`.
   - The configuration file is included in this port and selected automatically
     by the Makefile.

3. **Contiki-NG:**
   ```bash
   git clone https://github.com/contiki-ng/contiki-ng.git
   cd contiki-ng
   git submodule update --init --recursive
   ```

### Building

```bash
# Navigate to any example
cd examples/hello-world

# Build for XIAO nRF54L15
make TARGET=nrf BOARD=nrf54l15/xiao

# Output will be in build/nrf/nrf54l15/xiao/
```

### Flashing

Connect the XIAO nRF54L15 via USB and flash:

```bash
make TARGET=nrf BOARD=nrf54l15/xiao hello-world.flash
```

The Makefile will automatically:
1. Generate a .hex file from the .elf
2. Use OpenOCD with the board-specific config
3. Flash via CMSIS-DAP
4. Verify and reset the board

**Flashing Tool Priority:**
1. OpenOCD (recommended for XIAO - uses onboard CMSIS-DAP)
2. JLinkExe (if external J-Link connected)
3. nrfjprog (if J-Link tools installed)
4. nrfutil (fallback)

### Serial Console

Connect to the board's serial port (115200 baud, 8N1):

```bash
# macOS
screen /dev/tty.usbmodem* 115200

# Linux
screen /dev/ttyACM0 115200

# Or use any serial terminal (minicom, picocom, etc.)
```

You should see output like:
```
Starting Contiki-NG
 Net: sicslowpan
 MAC: CSMA
Contiki-NG started
Hello, world
```

## Hardware Details

### Pin Mappings

The XIAO nRF54L15 pin mappings are defined in `xiao-nrf54l15-def.h`:

```c
// LEDs
#define XIAO_NRF54L15_LED_PORT       2
#define XIAO_NRF54L15_LED_PIN        0   // P2.0 - User LED

// RF front-end switch
#define XIAO_NRF54L15_RF_SW_PWR_PORT 2
#define XIAO_NRF54L15_RF_SW_PWR_PIN  3   // P2.3
#define XIAO_NRF54L15_RF_SW_SEL_PORT 2
#define XIAO_NRF54L15_RF_SW_SEL_PIN  5   // P2.5

// Buttons
#define XIAO_NRF54L15_BUTTON_PORT    0
#define XIAO_NRF54L15_BUTTON_PIN     0   // P0.0 - User button

// UART console (routed to onboard SAMD11 USB CDC)
#define XIAO_NRF54L15_UART_INSTANCE  20  // UART20
#define XIAO_NRF54L15_UART_TX_PORT   1
#define XIAO_NRF54L15_UART_TX_PIN    9   // P1.9
#define XIAO_NRF54L15_UART_RX_PORT   1
#define XIAO_NRF54L15_UART_RX_PIN    8   // P1.8
```

### OpenOCD Configuration

The board-specific OpenOCD configuration is in `support/openocd.cfg`. Key features:
- Uses CMSIS-DAP interface (onboard debugger)
- Configures SWD transport
- Defines `nrf54l-load` procedure for proper flash writing
- Sets up AUX-AP for debugging

The configuration is adapted from Zephyr's XIAO nRF54L15 support.

## Technical Notes

### GRTC Timer Implementation

The nRF54L15 has several views into the Global Real-Time Counter (GRTC).
This port uses the application-core GRTC IRQ for both clock and rtimer.

- The MDK defines `GRTC_IRQn` as `GRTC_0_IRQn` by default.
- The nrfx HAL then redefines `GRTC_IRQn` to `GRTC_2_IRQn` for
  `NRF_APPLICATION && !NRF_TRUSTZONE_NONSECURE` (i.e., the secure
  application core).
- `nrfx_glue.h` undefines the MDK default before the HAL header is
  pulled in so the redefinition compiles cleanly without error.

`clock-arch.c` and `rtimer-arch.c` use the resulting `GRTC_IRQn`
(which resolves to `GRTC_2_IRQn` on the application core).

### GPIO/GPIOTE v3.x API

The nRF54L15 has two GPIOTE instances:
- **GPIOTE30:** For Port 0 (P0.x pins)
- **GPIOTE20:** For Port 1 (P1.x pins)

The nrfx v3.x API requires instance pointers. We provide wrapper functions:
- `gpio_hal_arch_interrupt_enable_nrfx_v3()`
- `gpio_hal_arch_interrupt_disable_nrfx_v3()`

These automatically select the correct GPIOTE instance based on the pin's port.

### Radio and Networking

The 802.15.4 radio is driven by Nordic's `nrf_802154` library (pulled in as the
`sdk-nrfxlib` submodule). The Contiki-NG wrapper is in
`arch/cpu/nrf/nrf54l15/nrf-ieee-driver-nrf54l15.c`. IPv6 and CSMA are enabled by
default in `nrf54l15-conf.h`:

```c
#define NETSTACK_CONF_WITH_IPV6 1
#define QUEUEBUF_CONF_NUM 4
```

## Troubleshooting

### Build Issues

**Problem:** `GRTC_IRQn redefined` warning/error
- **Solution:** Already fixed in `nrfx_glue.h` - update to latest code

**Problem:** `nrfx_gpiote_trigger_enable` wrong arguments
- **Solution:** Already fixed with wrapper functions - update to latest code

**Problem:** Linker errors about networking functions
- **Solution:** Make sure the `sdk-nrfxlib` submodule is initialised
  (`git submodule update --init --recursive`); the `nrf_802154` driver lives
  there.

### Flashing Issues

**Problem:** OpenOCD can't find target config
- **Solution:** The config is in `support/openocd.cfg` - it's automatically used by the Makefile

**Problem:** OpenOCD reports connection failure
- **Check:** USB cable is connected
- **Check:** Board is powered on (LED should light up)
- **Check:** No other program is using the USB port
- **Try:** Unplug and replug the USB cable

**Problem:** `JLinkExe` tries to flash but fails
- **Solution:** The XIAO has CMSIS-DAP (not J-Link). Use OpenOCD or disable JLink in PATH temporarily

### Runtime Issues

**Problem:** No serial output
- **Check:** Baudrate is 115200
- **Check:** Correct serial port selected
- **Try:** Reset the board (press reset button or re-flash)

**Problem:** LEDs don't work
- **Check:** Pin P2.0 is correctly defined in the board config (see
  `xiao-nrf54l15-def.h`)
- **Try:** Use the `leds-example` to test LED functionality

## Known Limitations

1. **No Low-Power:** Low-power modes not tested/optimized
2. **OpenOCD for flashing:** The XIAO has no J-Link, so flashing goes through
   its onboard CMSIS-DAP via OpenOCD (stock 0.12.0+) rather than nrfjprog
3. **No Watchdog/Temperature Sensor:** Integration pending

## Development Roadmap

### Phase 1: Basic Functionality ✅
- [x] Build system
- [x] GRTC timers
- [x] GPIO/GPIOTE
- [x] UART console
- [x] OpenOCD flashing
- [x] hello-world example

### Phase 2: Radio Driver ✅
- [x] Integrate Nordic `nrf_802154` driver via `sdk-nrfxlib`
- [x] TX/RX verified on-device
- [x] ACK handling via CSMA

### Phase 3: Networking ✅
- [x] IPv6 stack enabled
- [x] RPL routing
- [x] UDP examples (`rpl-udp`)

### Phase 4: Power Optimization
- [ ] Low-power mode support
- [ ] Sleep/wake testing
- [ ] Power profiling
- [ ] Battery operation validation

### Phase 5: Additional Features
- [ ] Watchdog driver
- [ ] Temperature sensor
- [ ] ADC support
- [ ] Additional examples

## Contributing

Found an issue or want to contribute? Here's how:

1. **Report bugs:** Open an issue with detailed info (build log, hardware version, etc.)
2. **Submit fixes:** Create a PR with clear description of changes
3. **Add features:** Discuss in issues first, then submit PR
4. **Improve docs:** Documentation PRs always welcome!

## References

- **Board Info:** https://wiki.seeedstudio.com/xiao_nrf54l15/
- **nRF54L15 Datasheet:** https://www.nordicsemi.com/Products/nRF54L15
- **Contiki-NG Docs:** https://docs.contiki-ng.org/
- **nrfx Repository:** https://github.com/NordicSemiconductor/nrfx
- **Zephyr XIAO Support:** https://docs.zephyrproject.org/latest/boards/seeed/xiao_nrf54l15/

## License

This port is licensed under the same terms as Contiki-NG (3-clause BSD license).

## Acknowledgments

- Seeed Studio for the XIAO nRF54L15 board
- Nordic Semiconductor for nRF54L15 SoC and nrfx HAL
- Zephyr Project for OpenOCD configuration reference
- Contiki-NG community for the framework

---

**Maintainer:** Joakim Eriksson <joakim.eriksson@ri.se>
**Status:** Alpha - core functionality, 802.15.4 radio, and IPv6 networking working

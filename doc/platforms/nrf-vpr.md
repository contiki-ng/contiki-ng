# nrf-vpr: Nordic Semiconductor nRF54L15 FLPR (RV32EMC coprocessor)

This guide describes the Contiki-NG port for the **FLPR** (Fast Lightweight Peripheral Processor) — the RISC-V VPR coprocessor on the nRF54L15. The FLPR is an RV32EMC core — the RV32E base (16 GP registers, no F/D extensions) plus the M (hardware 32-bit multiply/divide) and C (compressed-instruction) extensions — with its own tightly-coupled SRAM/RRAM partitions. It does not run by itself; the Cortex-M33 application core loads its firmware into SRAM and releases it from reset at run time.

Hardware-validated on both the nRF54L15-DK (PCA10156) and the Seeed XIAO
nRF54L15 (`BOARD=nrf54l15/xiao`; see *Boards* below).

## Port Features

The following features are implemented:

* Contiki-NG process scheduler, etimer, ctimer running on the FLPR
* `clock_time()` driven by GRTC SYSCOUNTER — 1 ms resolution at real wall-clock rate
* GPIO write access from the FLPR (LED demo: the user LED on GPIO port 2 toggles at 1 Hz; pin is board-dependent)
* M33-side companion app that embeds the FLPR blob, performs the SPU + VPR boot dance, polls a shared counter, and (on boards with a second LED) blinks it
* Resulting FLPR firmware is ~8 KB (kernel + one process)

Not yet implemented:

* Interrupt-driven etimer compare (GRTC CC channel 3) — currently polled in the main loop
* rtimer (GRTC CC channel 4 reserved for it)
* 802.15.4 radio on the FLPR — on this port the radio remains on the M33

## Prerequisites and Setup

Two toolchains are needed because the M33 and FLPR sides build separately and the M33 image embeds the FLPR binary.

### M33 side

Same as the existing `nrf` port:

* `arm-none-eabi-gcc` (Homebrew `gcc-arm-embedded` or system package)
* `gmake` (GNU Make 4+ — Apple's bundled make 3.81 is too old)
* For flashing: OpenOCD 0.12.0+ (Homebrew `openocd` or system package), or
  J-Link tools (`nrfjprog`/`JLinkExe`). The Seeed XIAO flashes over its onboard
  CMSIS-DAP via OpenOCD with no extra setup — stock OpenOCD works because the
  board config writes RRAM with `load_image` and never invokes an OpenOCD flash
  driver, so no nRF54L15 flash-driver patch is required. The DK has an onboard
  SEGGER J-Link and is flashed with the J-Link tools (no OpenOCD config ships
  for it in-tree).

### FLPR side

* A bare-metal RISC-V GCC toolchain that ships **both** an `rv32emc`/`ilp32e`
  multilib (libgcc — the FLPR has the M extension for hardware 32-bit
  multiply/divide, but libgcc is still needed for 64-bit division, e.g.
  `__udivdi3` in `clock_time()`) **and** a C library (newlib — the kernel
  includes `<inttypes.h>`). GCC 12+ is needed for the `zicsr`/`zifencei`
  extension strings (14.3+ recommended).

  > A compiler-only GCC without newlib or the `rv32emc` multilib (for example
  > Homebrew's `riscv64-elf-gcc`) will compile but fail to link/headers. Verify a
  > candidate with `<prefix>-gcc -print-multi-lib | grep ilp32e` (must print an
  > rv32e-family variant such as `rv32emc`) and `<prefix>-gcc -print-file-name=libc.a` (must be a real path).

  Two known-good options:

  * **xPack `riscv-none-elf-gcc`** — prebuilt, includes the rv32emc multilib and
    newlib. Install via `npm i -g @xpack-dev-tools/riscv-none-elf-gcc` or the
    release tarball, then build with `RISCV_PREFIX=riscv-none-elf RISCV_PATH=<dir>`.
  * **Prebuilt SDK** at `riscv64-zephyr-elf` layout:

        ZSDK_VER=v1.0.1
        curl -L -o /tmp/zsdk-riscv.tar.xz \
            https://github.com/zephyrproject-rtos/sdk-ng/releases/download/$ZSDK_VER/toolchain_gnu_macos-aarch64_riscv64-zephyr-elf.tar.xz
        mkdir -p ~/zephyr-toolchain-riscv
        tar -xJf /tmp/zsdk-riscv.tar.xz -C ~/zephyr-toolchain-riscv --strip-components=1

  The FLPR build defaults to `RISCV_PATH=~/zephyr-toolchain-riscv` and
  `RISCV_PREFIX=riscv64-zephyr-elf`; override either (see *Compilation Options*).

* Python 3 for the blob-embedding script (`tools/flpr-blob-gen.py`).

## Build and Deploy

End-to-end on an nRF54L15-DK (PCA10156). One `flpr-host` build produces a
single M33 image with the FLPR firmware embedded in it — you flash one image.

**1. Install the toolchains** (see *Prerequisites and Setup* above): the
   `arm-none-eabi` toolchain for the M33 and an rv32emc-capable RISC-V GCC for the
   FLPR (default `RISCV_PREFIX=riscv64-zephyr-elf` at `~/zephyr-toolchain-riscv`;
   select another with `RISCV_PREFIX=`/`RISCV_PATH=`).

**2. Build and flash** — a single command from `examples/platform-specific/nrf/flpr-host`:

    cd examples/platform-specific/nrf/flpr-host
    gmake TARGET=nrf BOARD=nrf54l15/dk WERROR=0 flpr-host.flash

   For the Seeed XIAO nRF54L15, use `BOARD=nrf54l15/xiao` instead. The
   Makefile selects the correct FLPR LED pin per board and only blinks the
   M33's second LED on boards that have one (the DK).

   The `flpr-host` Makefile transparently rebuilds the FLPR firmware
   (`../hello-vpr/build/nrf-vpr/hello-vpr.bin`) with the RISC-V toolchain (and
   the board-appropriate FLPR LED pin) and
   regenerates the embedded blob header (`flpr-blob.h`) before the M33 build
   runs, so you never invoke the FLPR build by hand for the demo.

   To build the two images separately instead, see *Compilation Targets* below.

**3. Open the serial console:**

    gmake TARGET=nrf BOARD=nrf54l15/dk WERROR=0 PORT=/dev/cu.usbmodem* login

**4. Verify** — you should see:

* The FLPR-driven user LED on GPIO port 2 blinking at **1 Hz** (DK: LED0 = gpio2.9; XIAO: user LED = gpio2.0)
* On the DK only, **LED1** (gpio1.10) blinking at **2 Hz** — driven by the M33 (the XIAO has a single user LED, so the M33 has no separate LED there)
* On the serial console (`make ... PORT=/dev/cu.usbmodem* login`):

      [INFO: flpr-host ] M33 boot complete, blob=8252 bytes
      [INFO: flpr-host ] Blob memcpy'd to 0x20028000
      [INFO: flpr-host ] SPU PERIPH[12] before=0x8001000a after=0x8001001a
      [INFO: flpr-host ] M33 is in SECURE mode (SPU S access succeeded)
      [INFO: flpr-host ] VPR_S after launch: INITPC=0x20028000 CPURUN=0x1
      [INFO: flpr-host ] [FLPR] tick 2
      [INFO: flpr-host ] [FLPR] tick 4
      ...

`tick` advances by exactly 2 per second, confirming the GRTC-driven Contiki kernel is running on the FLPR at real wall-clock rate.

## Examples

| Example                    | Target              | Purpose                                                                                                                                                             |
| -------------------------- | ------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `examples/platform-specific/nrf/hello-vpr` | `nrf-vpr`           | Smallest possible Contiki-NG running on the FLPR: one process, etimer, LED blink, tick counter in shared SRAM. Build output is the FLPR firmware blob.              |
| `examples/platform-specific/nrf/flpr-host` | `nrf` (`nrf54l15/dk` or `nrf54l15/xiao`) | M33-side companion. Embeds the FLPR blob, runs the SPU + VPR boot dance, polls the shared counter, blinks a second LED (DK only), prints over UART.   |

## Compilation Targets

The FLPR side uses TARGET `nrf-vpr` and has no BOARD selector of its own; its
only board dependency is the user-LED pin, passed in as `LED0_PIN` (default
P2.00; the DK uses P2.09). The M33 side selects the board with `BOARD` as usual,
and `flpr-host` forwards the matching `LED0_PIN` to the FLPR sub-make
automatically (see *Boards* below).

    # FLPR firmware (default LED pin = P2.00; pass LED0_PIN=9 for the DK)
    gmake TARGET=nrf-vpr WERROR=0
    gmake TARGET=nrf-vpr LED0_PIN=9 WERROR=0

    # M33 firmware (uses existing nrf port)
    gmake TARGET=nrf BOARD=nrf54l15/dk WERROR=0 <project>.flash
    gmake TARGET=nrf BOARD=nrf54l15/xiao WERROR=0 <project>.flash
    gmake TARGET=nrf BOARD=nrf54l15/dk WERROR=0 PORT=/dev/cu.usbmodem* login

## Boards

| Board | `BOARD` | FLPR LED (1 Hz) | M33 LED (2 Hz) |
| ----- | ------- | --------------- | -------------- |
| nRF54L15-DK (PCA10156) | `nrf54l15/dk`   | LED0 = P2.09 | LED1 = P1.10 |
| Seeed XIAO nRF54L15    | `nrf54l15/xiao` | user LED = P2.00 | none (single user LED) |

The XIAO exposes a single user LED (P2.00), which the FLPR drives. There is no
second LED for the M33, so its blinker is compiled out there
(`FLPR_HOST_M33_LED` is set by the Makefile only for the DK); the M33 remains
observable through its serial `[FLPR] tick` logging.

## Compilation Options

* `RISCV_PATH=<dir>` — RISC-V toolchain install directory (default: `~/zephyr-toolchain-riscv`).
* `RISCV_PREFIX=<prefix>` — tool prefix (default: `riscv64-zephyr-elf`). For example, `RISCV_PREFIX=riscv-none-elf` selects an xPack toolchain.
* `TOOLCHAIN_BIN=<dir>/bin/<prefix>-` — set the full tool path/prefix directly, bypassing the two above.
* `ZSDK_RISCV=<path>` — backward-compatible alias for `RISCV_PATH`.
  The toolchain must ship an `rv32emc`/`ilp32e` multilib (libgcc) and a C library (newlib); a compiler-only GCC without those will not link the FLPR.
* `WERROR=0` — currently required for the M33 build because of an RWX-segment warning from nrfx's M33 linker script. The FLPR build sets `-Wl,--no-warn-rwx-segments` directly so `WERROR` does not need to be relaxed.

## How it boots

The M33 launches the FLPR with the boot sequence the nRF54L15 requires:

    /* 1. Copy the FLPR binary into its execution memory (start of the   */
    /*    96 KB SRAM block the FLPR owns). */
    memcpy((void *)0x20028000, flpr_blob, flpr_blob_len);

    /* 2. Set VPR00's SPU PERIPHACCESS.SECATTR=Secure.                  */
    /*    Without this, INITPC/CPURUN writes succeed and read back, but */
    /*    the VPR never actually fetches a single instruction.          */
    NRF_SPU00_S->PERIPH[12].PERM |= (1u << 4);

    /* 3. Tell the VPR where to start.                                  */
    NRF_VPR00_S->INITPC = 0x20028000;

    /* 4. Release the VPR from reset.                                   */
    NRF_VPR00_S->CPURUN = 1;

All four writes use Secure addresses. Contiki on the nRF54L15 M33 runs in Secure mode by default, so no TrustZone transition is required.

## Implementation notes

Things worth knowing before modifying this port.

### nrfx startup must clear BSS

`arch/cpu/nrf/lib/nrfx/mdk/gcc_startup_nrf54l15_flpr.S` only emits the `.bss` / `.sbss` / `.tbss` zero-init loops when `__STARTUP_CLEAR_BSS` is defined. Without it, Contiki globals (`process_list`, `timerlist`, ...) hold whatever garbage was in SRAM and the kernel hangs the first time it walks one of those lists. `arch/cpu/nrf-vpr/Makefile.nrf-vpr` defines this.

### GRTC must be read via the Secure address

The M33 keeps GRTC's SPU PERIPHACCESS set to Secure, so the NS aperture (`0x400E2000`) faults from the FLPR with `mcause=5` (load access fault). Use `NRF_GRTC_S` (`0x500E2000`). `arch/cpu/nrf-vpr/clock-arch.c` does.

### Channel allowlist override for nrfx-grtc

The stock `nrfx_config_nrf54l15_flpr.h` template hard-codes `NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK = 0xF0` (channels 4..7). That collides with M33's owned 5 and 6, with the zero-latency channel 7, and it omits channel 3 entirely. The Nordic split for nRF54L15 is M33: 0,1,2,5,6 / FLPR: 3,4. `Makefile.nrf-vpr` overrides the mask to `0x18`.

### Radio is on the M33, not the FLPR

The nRF54L15 802.15.4 backend uses TIMER20 and TIMER10 (`nrf_802154_platform_sl_lptimer.c`, `nrf_802154_platform_timestamper.c`), not GRTC. So the M33 radio stack and the FLPR clock do not contest.

### Trap handler

The nrfx-provided `Trap_Handler` is a silent infinite loop. `arch/cpu/nrf-vpr/startup-stubs.c` reroutes `mtvec` to a handler that writes `0xFA1100 | mcause` to `0x2003F000` and `mepc` to `0x2003F004` before spinning, so any CPU exception is visible from the M33-side console instead of silently hanging the FLPR.

When `flpr-host` prints `[FLPR] counter 0xFA1100XX mepc=0xYYYYYYYY` instead of an
advancing `tick`, the FLPR took an exception: `XX` is the RISC-V `mcause` and
`mepc` is the faulting PC. Common causes:

| `mcause` | Meaning |
| -------- | ------------------- |
| `2`      | Illegal instruction |
| `5`      | Load access fault   |
| `7`      | Store access fault  |

A load/store access fault (5/7) most often means a peripheral was accessed
through its Non-Secure alias — see *GRTC must be read via the Secure address*
above.

### Bringing up a new board

When porting to a board other than the DK or XIAO, confirm the M33-side boot dance
actually starts the VPR *before* debugging the full kernel. Replace the FLPR
blob with this five-instruction RISC-V "stamp" — it writes a known marker to the
shared counter and spins:

    .section .startup, "ax"
    .global Reset_Handler
    Reset_Handler:
        li   t0, 0x2003F000     /* counter address          */
        lui  t1, 0xCAFEC
        addi t1, t1, -0x542     /* t1 = 0xCAFEBABE          */
        sw   t1, 0(t0)          /* [0x2003F000] = 0xCAFEBABE */
    1:  j    1b

Assemble it with the RISC-V toolchain (`-march=rv32emc -mabi=ilp32e`), point
`tools/flpr-blob-gen.py` at the resulting `.bin`, and flash `flpr-host`. If the
M33 reads back `0xCAFEBABE` from `0x2003F000`, INITPC/CPURUN and the SPU
SECATTR step are correct and you can move on to the real firmware. If it stays
`0`, the VPR is not fetching instructions — re-check the execution-memory base
address and the SPU `PERIPH[12]` SECATTR write.

## Known limitations

* `etimer` polling rather than CC-interrupt driven — the FLPR busy-loops between events. CC channel 3 wiring is the next major commit.
* `rtimer` is a stub. CC channel 4 will drive it later.
* No FLPR-side UART output — the FLPR shares its `printf` channel with the M33 only through the shared counter region. A proper IPC mailbox is a future commit.

## References

* Zephyr `drivers/misc/nordic_vpr_launcher/nordic_vpr_launcher.c` — canonical boot helper
* Zephyr `dts/vendor/nordic/nrf54l15_cpuflpr.dtsi` — FLPR memory layout
* Zephyr `snippets/nordic/nordic-flpr/soc/nrf54l15_cpuapp.overlay` — execution-memory / source-memory addresses
* nrfx `mdk/gcc_startup_nrf54l15_flpr.S`, `mdk/nrf54l15_xxaa_flpr.ld`, `hal/nrf_vpr.h`, `hal/nrf_spu.h`, `hal/nrf_grtc.h`

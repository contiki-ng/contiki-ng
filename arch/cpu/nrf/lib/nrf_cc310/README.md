# CryptoCell-310 (CC310) hardware ECC backend for `lib/ecc.h`

This is the Contiki-NG source that routes the `lib/ecc.h` ECC service onto the
nRF52840 Arm CryptoCell-310 (CC310) hardware accelerator:

- `../../dev/nrf52840-cc310-ecc.c` — the backend (implements `lib/ecc.h`).
- `src/nrf_cc3xx_platform_contiki.c`, `src/nrf_cc3xx_alloc_contiki.c` — the glue
  the cc3xx PSA driver needs from Contiki (external RNG, allocator, ABI shims).
- `mbedtls-config-cc310.h` — the minimal mbedTLS PSA config linked with the driver.
- `../../Makefile.cc310-crypto` — build wiring.

The CC310 libraries and headers are **not** vendored here; they come from the
in-tree `sdk-nrfxlib` submodule. Build an example with:

    git submodule update --init arch/cpu/nrf/lib/sdk-nrfxlib
    make -C examples/libs/ecc TARGET=nrf BOARD=nrf52840/dk ECC_HW_BACKEND=cc310 ecdsa

Leaving `ECC_HW_BACKEND` unset builds the default software (uECC) backend.

## Curve support

The backend currently enables **secp256r1 (NIST P-256) only**. `ecc_enable()`
returns an error for any other curve.

This is a limitation of *this backend*, not of the hardware. The CC310 `core`
library provides domain parameters for NIST P-192/224/256/384, the Koblitz
secp192k1/224k1/256k1, Brainpool-P256r1, Curve25519/X25519 and Ed25519.
(Ed25519 and X25519 use the EdDSA / X25519 primitives rather than the
ECDSA/ECDH-over-Weierstrass model of `lib/ecc.h`, and in this tree are handled
by separate code, not this backend.)

## Adding another Weierstrass curve (e.g. P-384, secp256k1)

The backend is driven by the `cc310_curves[]` descriptor table, and the cc3xx
PSA entry points (`cc3xx_generate_key`, `cc3xx_sign_hash`, `cc3xx_verify_hash`,
`cc3xx_key_agreement`, `cc3xx_export_public_key`) read the curve family and bit
size from the attributes. Adding a curve is therefore small and local:

1. **Curve object** — ensure `os/lib/ecc-curve.h` defines an `ecc_curve_t` for
   the curve (add one if needed).
2. **Descriptor row** — add an entry to `cc310_curves[]` in
   `dev/nrf52840-cc310-ecc.c` with the curve's field-element size, PSA family
   (`PSA_ECC_FAMILY_SECP_R1`, `_SECP_K1`, …), key bit length, the ECDSA algorithm
   (`PSA_ALG_ECDSA(PSA_ALG_SHA_xxx)` matching the curve's hash), and the uECC
   curve used for software validation / decompression. For a curve uECC does not
   implement (e.g. P-384, Brainpool), supply an alternative on-curve check and
   point decompression.
3. **Buffer size** — if the curve's field is larger than P-256, raise
   `CC310_MAX_ELEMENT_BYTES`.
4. **Tests** — add known-answer / round-trip tests (see `examples/libs/ecc`).

No change is needed to the vendored libraries or the submodule pin: the CC310
`core` / `psa_crypto` libraries already contain the other curves' domain
parameters.

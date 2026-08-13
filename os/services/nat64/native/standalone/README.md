# Standalone native NAT64 translator

Lets a single native node reach IPv4 hosts through NAT64 with no border router
and no `tun` device (hence no `sudo`). It is the same mechanism the RPL border
router uses for NAT64 — a uIP fallback interface (`nat64_standalone_interface`)
that hands the node's own `64:ff9b::/96` (RFC 6052) traffic to the NAT64
service, which translates it over ordinary host IPv4 sockets — but without the
border router, the mesh, or the `tun`.

## Use

Build the module into a native application; that is the whole opt-in:

```makefile
ifeq ($(TARGET),native)
MODULES += os/services/nat64/native/standalone
endif
```

The module's `module-macros.h` is force-included into every translation unit
(`-imacros`), so the application needs no configuration and no code. It wires
`UIP_FALLBACK_INTERFACE` to the standalone interface, suppresses the platform's
default route, and enables NAT64. The application then just sends to a
`64:ff9b::<IPv4>` address. Native target only.

## Design notes

- **NAT64 becomes the node's only off-link path.** uIP's fallback interface
  fires only for packets with no route *and* no default route, and there is no
  way to point a per-prefix route at it without an on-link next hop. So the
  module must suppress the platform's default route to `PREFIX::1`
  (`NATIVE_WITH_IPV6_DEFAULT_ROUTE=0`), and the fallback's `output()` drops any
  off-link destination that is not a NAT64 address. This is exactly the
  intended scenario — a lone node with no border router — but it does mean such
  a node cannot also reach other IPv6 networks through some upstream.

- **Mutually exclusive with the RPL border router module.** Both define
  `UIP_FALLBACK_INTERFACE`; a binary uses one or the other, not both.

- **NAT64 is enabled at compile time** (`NAT64_DEFAULT_ENABLED=1`), to stay
  consistent with the compile-time route suppression: a build that suppressed
  the default route but left NAT64 off (the `--nat64` default) could reach
  nothing off-link. There is therefore no run-time switch to turn NAT64 off in
  a standalone build. A plain `os/services/nat64/native` build (e.g. the border
  router) is unaffected and still defaults to off until `--nat64`.

## DNS

The module gives raw IPv4 reachability only; there is no name resolution, so
applications address servers by IPv4 literal embedded in the NAT64 prefix. For
name resolution see the DNS64 support in `os/services/nat64`.

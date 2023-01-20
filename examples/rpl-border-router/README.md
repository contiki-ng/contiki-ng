# rpl-border-router

This is the Contiki-NG border router. It supports two main modes of operation:
embedded and native. In both cases, the border router runs a simple Web server
that exposes a list of currently connected nodes via HTTP.

See the [RPL border router tutorial](https://docs.contiki-ng.org/en/develop/doc/tutorials/RPL-border-router.html)

## Embedded border router

The embedded border router runs on a node. It is connected to the host via SLIP.
The host simply runs a tun gateway (`tunslip6`). To use, program a node, and
then start `tunslip6` on the host via the make command `connect-router`.
See `embedded/README.md` for more.

## Native border router

The native border router runs directly at the host. The node simply runs a
SLIP-radio interface (`examples/slip-radio`). The host, on the other hand, runs
a full 6LoWPAN stack.
See `native/README.md` for more.

## RPL node

As RPL node, you may use any Contiki-NG example with RPL enabled, but which
does not start its own DAG (as this is the responsibility of the border router).
For instance `examples/hello-world` or `examples-coap` are great starting
points. This is not intended to run with `examples/rpl-udp` however, as this
examples builds its own stand-alone, border-router-free RPL network.

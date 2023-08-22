# Press release

We are releasing version 5.0 of Contiki-NG. This release adds XXX.

## Find out more at:

* GitHub repository: https://github.com/contiki-ng/contiki-ng
* Documentation: https://contiki-ng.readthedocs.io/en/release-v5.0
* Web site: http://contiki-ng.org
* Nightly testbed runs: https://contiki-ng.github.io/testbed

## Engage with the community:

* Contiki-NG tag on Stack Overflow: https://stackoverflow.com/questions/tagged/contiki-ng
* Github Discussions: https://github.com/contiki-ng/contiki-ng/discussions
* Gitter: https://gitter.im/contiki-ng
* Twitter: https://twitter.com/contiki_ng

The Contiki-NG team

## API changes for ports outside the main tree

### Centralized clock_time_t definition

Ports should now define `CLOCK_CONF_SIZE` instead of a typedef for `clock_time_t`
([#2550](https://github.com/contiki-ng/contiki-ng/pull/2550)).

This is similar to how `RTIMER_CONF_CLOCK_SIZE` is already handled.

## Cooja API changes for plugins outside the main tree

## Changelog

### Contiki-NG

* Mac M1/M2 support for Cooja platform ([#2486](https://github.com/contiki-ng/contiki-ng/pull/2486))

All [commits](https://github.com/contiki-ng/contiki-ng/compare/release/v4.9...develop) since v4.9.

### Cooja

* XXX

All [commits](https://github.com/contiki-ng/cooja/compare/33d41ae9f8...master) since v4.9.

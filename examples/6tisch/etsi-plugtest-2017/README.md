# 6tisch/etsi-plugtest-2017

## The 1st F-Interop 6TiSCH Interoperability Event

### Overview

This project was used to build firmwares for [the 1st F-Interop 6TiSCH
Interoperability
Event](http://www.etsi.org/news-events/events/1197-6tisch-interop-prague-2017),
which worked well in all the tests except for "secjoin".

### Authors

* Simon Duquennoy
* Yasuyuki Tanaka

### Supported Hardwares

The following hardwares were used in the event:

* Zolertia Remote (TARGET=`zoul`, BOARD=`remote`)
* JN156x (TARGET=`jn516`)
* CC2650 LaunchPad (TARGET=`cc26x0-cc13x0`, BOARD=`launchpad/cc2650`)

### Usage

Access to your target board through serial connection. You'll get available
commands by hit `help` on the shell prompt.

```shell
> help
Available commands:
'> help': Shows this help
'> ip-addr': Shows all IPv6 addresses
'> ip-nbr': Shows all IPv6 neighbors
'> log module level': Sets log level (0--4) for a given module (or "all"). For module "mac", level 4 also enables per-slot logg'> ping addr': Pings the IPv6 address 'addr'
'> rpl-set-root 0/1 [prefix]': Sets node as root (on) or not (off). A /64 prefix can be optionally specified.
'> rpl-status': Shows a summary of the current RPL state
'> rpl-global-repair': Triggers a RPL global repair
'> rpl-local-repair': Triggers a RPL local repair
'> routes': Shows the route entries
'> tsch-schedule': Shows the current TSCH schedule
'> tsch-status': Shows a summary of the current TSCH state
'> reboot': Reboot the board by watchdog_reboot()
'> 6top help': Shows 6top command usage
```

Your board runs as a 6TiSCH node by default. Its role can be changed to DAG root
by `rpl-set-root 1`.

You can see how it works with `test-with-cooja-mote.csc`.

### Configuration

Edit `project-conf.h` if necessary.

* `UIP_CONF_IPV6_CHECKS`: set 0 if you want to disable checksum validation
* `SIXP_CONF_WITH_PAYLOAD_TERMINATION_IE`: set 1 if you want to append Paload Termination IE in 6P frames

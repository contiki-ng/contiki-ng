# TSCH and 6TiSCH

## Overview

Time Slotted Channel Hopping (TSCH) is a MAC layer of [IEEE 802.15.4-2015][ieee802.15.4-2015].
[6TiSCH][ietf-6tisch-wg] is an IETF Working Group focused on IPv6 over TSCH.
This is a Contiki-NG implementation of TSCH and the 6TiSCH so-called "minimal configuration",
which defines how to run a basic RPL+TSCH network.

It was developed by:
* Simon Duquennoy, SICS, simon.duquennoy@ri.se, github user: [simonduq](https://github.com/simonduq)
* Beshr Al Nahas, SICS (now Chalmers University), beshr@chalmers.se, github user: [beshrns](https://github.com/beshrns)
* Atis Elsts, Univ. Bristol (now EDI), atis.elsts@edi.lv, github user: [atiselsts](https://github.com/atiselsts)

This implementation is presented in depth and evaluated in our paper: [*TSCH and 6TiSCH for Contiki-NG: Challenges, Design and Evaluation*](http://www.simonduquennoy.net/papers/duquennoy17tsch.pdf), IEEE DCOSS'17.
The scheduler Orchestra is detailed in [*Orchestra: Robust Mesh Networks Through Autonomously Scheduled TSCH*](http://www.simonduquennoy.net/papers/duquennoy15orchestra.pdf), ACM SenSys'15.

More documentation on:
* [Orchestra](/doc/programming/Orchestra)
* [The 6top sub-layer](/doc/programming/6TiSCH-6top-sub-layer)

## Getting started

* [TSCH tutorial in Contiki-NG](/doc/tutorials/TSCH-and-6TiSCH)
* [Switching Contiki-NG applications to TSCH](/doc/tutorials/Switching-to-TSCH)
* [Contiki-NG TSCH example applications](/doc/programming/TSCH-example-applications)

## Features

This implementation includes:
  * Standard IEEE 802.15.4-2015 frame version 2
  * Standard TSCH joining procedure with Enhanced Beacons with the following Information Elements:
    * TSCH synchronization (join priority and ASN)
    * TSCH slotframe and link (basic schedule)
    * TSCH timeslot (timeslot timing template)
    * TSCH channel hopping sequence (hopping sequence template)
  * Standard TSCH link selection and slot operation (10ms slots by default)
  * Standard TSCH synchronization, including with ACK/NACK time correction Information Element
  * Standard TSCH queues and CSMA-CA mechanism
  * Standard TSCH and 6TiSCH security
  * Standard 6TiSCH TSCH-RPL interaction (6TiSCH Minimal Configuration and Minimal Schedule)
  * A scheduling API to add/remove slotframes and links
  * A system for logging from TSCH timeslot operation interrupt, with postponed printout
  * Orchestra: an autonomous scheduler for TSCH+RPL networks
  * A drift compensation mechanism

## Platforms

The implemenation has been tested on the following platforms:
  * Tmote Sky (`sky`)
  * CC2538DK (`cc2538dk`)
  * Zolertia Z1 (`z1`)
  * Zolertia Zoul CC2538 (`zoul`)
  * Zolertia Zoul CC1200 (`zoul`, sub-GHz, with timeslots ranging between 5.8 and 31.5ms)
  * OpenMote-CC2538 (`openmote-cc2538`)
  * CC2650 (both `cc26x0-cc13x0` and `simplelink`)
  * CC2652R1 (`simplelink`)
  * CC1310 (both `cc26x0-cc13x0` and `simplelink`, sub-GHz, with 40ms timeslots)
  * CC1312R1 (`simplelink`, sub-GHz, with 40ms timeslots)
  * Cooja mote (`cooja`)

This implementation was present at the ETSI Plugtest
events in Prague in July 2015 and July 2017, and did successfully inter-operate with all
four implementations it was tested against.

This implementation can run with any network layer. In the IPv6+RPL case, `tsch-rpl.[ch]`, handles consistency between the RPL and TSCH topologies. However, some platforms (specifically, Tmote Sky) do not have sufficient ROM to support both TSCH and RPL, and others may require non-default parameters (reduced number of neighbors, routes, queue size) to fit in RAM.

## Code structure

The IEEE 802.15.4e-2012 frame format is implemented in:
* `os/net/mac/framer/frame802154.[ch]`: handling of frame version 2
* `os/net/mac/framer/frame802154-ie.[ch]`: handling of Information Elements

TSCH is implemented in:
* `os/net/mac/tsch/tsch.[ch]`: TSCH management (association, keep-alive), processes handling pending
outgoing and incoming packets, and interface with Contiki-NG's upper layers as a MAC driver. TSCH does not
require a RDC (nordc is recommended).
* `tsch-slot-operation.[ch]`: TSCH low-level slot operation, fully interrupt-driven. Node wake up at every active
slot (according to the slotframes and links installed), transmit or receive frames and ACKs. Received packets are
stored in a ringbuf for latter upper-layer processing. Outgoing packets that are dequeued (because acknowledged
or dropped) are stored in another ringbuf for upper-layer processing.
* `tsch-asn.h`: TSCH macros for Absolute Slot Number (ASN) handling.
* `tsch-packet.[ch]`: TSCH Enhanced ACK (EACK) and enhanced Beacon (EB) creation and parsing.
* `tsch-queue.[ch]`: TSCH  per-neighbor queue, neighbor state, and CSMA-CA.
* `tsch-schedule.[ch]`: TSCH slotframe and link handling, and API for slotframe and link installation/removal.
* `tsch-security.[ch]`: TSCH security, i.e. securing frames and ACKs from interrupt with ASN as part of the Nonce.
Implements the 6TiSCH minimal configuration K1-K2 keys pair (needs updating to latest minimal security draft).
* `tsch-rpl.[ch]`: used for TSCH+RPL networks, to align TSCH and RPL states (preferred parent -> time source,
rank -> join priority) as defined in the 6TiSCH minimal configuration.
* `tsch-log.[ch]`: logging system for TSCH, including delayed messages for logging from slot operation interrupt.
* `tsch-adaptive-timesync.c`: used to learn the relative drift to the node's time source and automatically compensate for it.
* `tsch-timeslot-timing.c`: defines TSCH timeslot timing templates.
* `tsch-const.h`: the constants required by TSCH.
* `tsch-types.h`: the data types defined by TSCH.
* `tsch-conf.h`: general configuration file for TSCH.

## Using TSCH

A simple TSCH+RPL example is included under `examples/6tisch/simple-node`.
To use TSCH, first make sure your platform supports it.
Currently, `sky`, `cc2538dk`, `zoul`, `openmote-cc2538`, `cc26x0-cc13x0`, and `cooja` are the supported platforms.
To add your own, we refer the reader to the next section.

To enable TSCH in your application, simple add the TSCH module from your Makefile with:
```
MAKE_MAC = MAKE_MAC_TSCH
```

To configure TSCH further, see the macros in `.h` files under `os/net/mac/tsch/` and redefine your own in your `project-conf.h`.

### Using TSCH with Security

To include TSCH standard-compliant security, set the following:
```c
/* Enable security */
#define LLSEC802154_CONF_ENABLED 1
```

The keys can be configured in `os/net/mac/tsch/tsch-security.h`.
Nodes handle security level and keys dynamically, i.e. as specified by the incoming frame header rather that compile-time defined.

By default, when including security, the PAN coordinator will transmit secured EBs.
Use `tsch_set_pan_secured` to explicitly ask the coordinator to secure EBs or not.

When associating, nodes with security included can join both secured or non-secured networks.
Set `TSCH_CONF_JOIN_SECURED_ONLY` to force joining secured networks only.
Likewise, set `TSCH_JOIN_MY_PANID_ONLY` to force joining networks with a specific PANID only.

### TSCH Scheduling

By default (see `TSCH_SCHEDULE_WITH_6TISCH_MINIMAL`), our implementation runs a 6TiSCH minimal schedule, which emulates an always-on link on top of TSCH.
The schedule consists in a single shared slot for all transmissions and receptions, in a slotframe of length `TSCH_SCHEDULE_DEFAULT_LENGTH`.

As an alternative, we provide [Orchestra](/doc/programming/Orchestra), an autonomous scheduling solution for TSCH where nodes maintain their own schedule locally, solely based on their local RPL state.

Finally, one can also implement his own scheduler, centralized or distributed, based on the scheduling API provides in `os/net/mac/tsch/tsch-schedule.h`.

### Configuring the association process

When attempting to associate to a network, nodes scan channels at random until they receive an enhanced beacon (EB).
The beaconing interval and the channel hopping sequence play a key role in how fast and efficient the association process is.
To fine-tune, experiment with the following defines:
* `TSCH_CONF_EB_PERIOD`: The initial (and minimal) EB period in clock ticks. Smaller values will result in more traffic but faster association.
* `TSCH_CONF_MAX_EB_PERIOD`: The maximum EB period. When running with RPL, the EB period is automatically mapped on the DIO period, but bounded by `TSCH_CONF_MAX_EB_PERIOD`. Here again, smaller values increase beaconing traffic for a faster association.
* `TSCH_CONF_DEFAULT_HOPPING_SEQUENCE`: The default hopping sequence (optionally, a coordinator could choose advertise a different sequence). Use a sequence with fewer different channels for faster association (but less frequency diversity).
* `TSCH_CONF_JOIN_HOPPING_SEQUENCE`: Optionally, set a different hopping sequence for scanning. Only use this if you also implement your own mechanism to restrict EBs to a subset of frequencies (e.g. using a slotframe of length 4 with one slot for EBs would result in hopping over only 4 channels).

## Porting TSCH to a new platform

Porting TSCH to a new platform requires a few new features in the radio driver, a number of timing-related configuration parameters.
The easiest is probably to start from one of the existing port: `sky`, `cc2538dk`, `zoul`, `openmote-cc2538`, `cc26x0-cc13x0`.

### Radio features required for TSCH

The main new feature required for TSCH is the so-called *poll mode*, a new Rx mode for Contiki-NG radio drivers.
In poll mode, radio interrupts are disabled, and the radio driver never calls upper layers.
Instead, TSCH will poll the driver for incoming packets, from interrupt, exactly when it expects one.

TSCH will check when initializing (in `tsch_init`) that the radio driver supports all required features, namely:
* get and set Rx mode (`RADIO_PARAM_RX_MODE`) as follows:
  * disable address filtering with `RADIO_RX_MODE_ADDRESS_FILTER`
  * disable auto-ack with `RADIO_RX_MODE_AUTOACK`
  * enable poll mode with `RADIO_RX_MODE_POLL_MODE`
* get and set Tx mode (`RADIO_PARAM_TX_MODE`) as follows:
  * disable CCA-before-sending with `RADIO_TX_MODE_SEND_ON_CCA`
* set radio channel with `RADIO_PARAM_CHANNEL`
* get last packet timestamp with `RADIO_PARAM_LAST_PACKET_TIMESTAMP`
* optionally: get last packet RSSI with `RADIO_PARAM_LAST_RSSI`
* optionally: get last packet LQI with `RADIO_PARAM_LAST_LQI`

### Timing macros required for TSCH

The following macros must be provided:
* `US_TO_RTIMERTICKS(US)`: converts micro-seconds to rtimer ticks
* `RTIMERTICKS_TO_US(T)`: converts rtimer ticks to micro-seconds
* `RADIO_DELAY_BEFORE_TX`: the delay between radio Tx request and SFD sent, in rtimer ticks
* `RADIO_DELAY_BEFORE_RX`: the delay between radio Rx request and start listening, in rtimer ticks
* `RADIO_DELAY_BEFORE_DETECT`: the delay between the end of SFD reception and the radio returning `1` to `receiving_packet()`
* `RADIO_PHY_OVERHEAD`: the number of header and footer bytes of overhead at the PHY layer after SFD, e.g. `3` for 802.15.4 in the 2.4 GHz spectrum (1 for the length field and 2 for the CRC)
* `RADIO_BYTE_AIR_TIME`: the air time for one byte in microsecond
* optionally, `TSCH_CONF_DEFAULT_TIMESLOT_TIMING`: the default TSCH timeslot timing, useful i.e. for platforms
slower or faster than 10ms timeslots (which are defined as `tsch_timeslot_timing_us_10000`).

## Per-slot logging

When setting the log level to `LOG_LEVEL_DBG`, or simply by directly enabling `TSCH_LOG_PER_SLOT`, one can get detailed logs. In fact, one (or two) line(s) for each slot in which a transmission (tx) or reception (rx) takes place. We detail the main types of logs here:
```
[INFO: TSCH-LOG  ] {asn-0.24c07 link-0-7-0-0 ch-20} bc-0-0 rx LL-0fcc->LL-NULL, len 35, seq 237, dr -1, edr 1
```

All TSCH logs are prefixed by information within `{}`. First comes the ASN (Absolute Slot Number). Then some information about the TSCH link: slotframe handle, slotframe length, time offset, channel offset. Last comes the physical channel (here, 20).

What follows is specific to slots where a reception (rx) happen: `bc/uc` stands for broadcast/unicast. The next bit indicates if this was a data packet or an EB. The next integer indicates the security level (0 means no security). Then comes 'rx' to indicate a reception. Then come, in a compact form, the source and destination addresses for this packet. Then follow the length of the packet and its MAC sequence number. Finally, `dr` indicates the drift correction that was applied when receiving, in rtimer ticks, and `edr` is the opposite of `dr`.

Let's look at a transmission:
```
[INFO: TSCH-LOG  ] {asn-0.306f3 link-0-7-0-0 ch-20} bc-0-0 tx LL-0fc4->LL-NULL, len 35, seq 188, st 0 1
```

The first part between `{}` is identical. Then comes the same information as above, `bc-0-0`. Then `tx` indicates a transmission. It is followed by the compact source and destination addresses. Then the length and sequence number. Finally, `st 0 1` indicates the status of the transmission. The first integer is a MAC status, as defined in `mac.h`. `0` indicates success. `2` indicates no-ack. The last integer indicates the transmission count for this particular packet. Here, this was the first (and only) attempt.

Now for a unicast transmission:
```
[INFO: TSCH-LOG  ] {asn-0.3591e link-0-7-0-0 ch-26} uc-1-0 tx LL-0fc4->LL-0fcc, len 32, seq 102, st 0 1, dr 1
```

This is almost identical to the case above, except for the ending `dr 1`. This shows the drift correction applied by the node, after receiving an ACK from its time source.

When communication goes wrong, you get something like:
```
[INFO: TSCH-LOG  ] {asn-0.38230 link-0-7-0-0 ch-15} uc-1-0 tx LL-0fc4->LL-0084, len 32, seq 103, st 2 1
[INFO: TSCH-LOG  ] {asn-0.38237 link-0-7-0-0 ch-20} uc-1-0 tx LL-0fc4->LL-0084, len 32, seq 103, st 2 2
[INFO: TSCH-LOG  ] {asn-0.38268 link-0-7-0-0 ch-15} uc-1-0 tx LL-0fc4->LL-0084, len 32, seq 103, st 2 3
[INFO: TSCH-LOG  ] {asn-0.38299 link-0-7-0-0 ch-25} uc-1-0 tx LL-0fc4->LL-0084, len 32, seq 103, st 2 4
[INFO: TSCH-LOG  ] {asn-0.38356 link-0-7-0-0 ch-26} uc-1-0 tx LL-0fc4->LL-0084, len 32, seq 103, st 2 5
[INFO: TSCH-LOG  ] {asn-0.383e9 link-0-7-0-0 ch-25} uc-1-0 tx LL-0fc4->LL-0084, len 32, seq 103, st 2 6
[INFO: TSCH-LOG  ] {asn-0.384b4 link-0-7-0-0 ch-15} uc-1-0 tx LL-0fc4->LL-0084, len 32, seq 103, st 2 7
[INFO: TSCH-LOG  ] {asn-0.3856a link-0-7-0-0 ch-26} uc-1-0 tx LL-0fc4->LL-0084, len 32, seq 103, st 2 8
```

Which shows eight consecutive failed attempts (`st 2`, and transmission count increasing from 1 to 8).

## Additional documentation

1. [IEEE 802.15.4-2015][ieee802.15.4-2015]
2. [IETF 6TiSCH Working Group][ietf-6tisch-wg]

[ieee802.15.4-2015]: https://standards.ieee.org/findstds/standard/802.15.4-2015.html
[ietf-6tisch-wg]: https://datatracker.ietf.org/wg/6tisch

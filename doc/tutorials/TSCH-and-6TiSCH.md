# TSCH and 6TiSCH

This tutorial will guide you through using TSCH and 6TiSCH (see [doc:tsch]). We will start from the `hello-world` example with the shell enabled (see [tutorial:shell]) and with MAC logs set to the maximum level (`LOG_CONF_LEVEL_MAC` set to `LOG_LEVEL_DBG`, see [tutorial:logging]). For existing examples using TSCH, see [Contiki-NG TSCH example applications](/doc/programming/TSCH-example-applications).

To enable TSCH, all you need to do is to set it as the MAC layer in the project Makefile:
```
MAKE_MAC = MAKE_MAC_TSCH
```

This will configure the system for TSCH, and if IPv6 and RPL is enabled (which is the default case), will also setup 6TiSCH, i.e. configure the network for smooth interaction between TSCH and IPv6. By default, it runs the 6TiSCH minimal schedule with slotframe length of `TSCH_SCHEDULE_DEFAULT_LENGTH` (use `viewconf` to find out your configuration).

As you have modified the Makefile, you need to clean:
```
$ make distclean
```

Compile your example again. Depending on the platform you are using, enabling TSCH and the shell might result in a too large RAM or ROM footprint. Read this tutorial to mitigate the issue: [tutorial:ram-rom-usage]. Now program two nodes, and pick one that you will use as coordinator. From the shell, set the node as RPL root; doing that will also automatically set it as TSCH coordinator, while the opposite is not true:
```
> rpl-set-root 1
```

The node will create a TSCH network and start advertising it through Enhanced Beacons (EBs). The other node should be scanning on all active channels. Once it receives an EB, it should join the network. Note that this can take a while, depending on the EB period (see `TSCH_EB_PERIOD` and `TSCH_MAX_EB_PERIOD`) and the channel hopping sequence (see `TSCH_DEFAULT_HOPPING_SEQUENCE` and `TSCH_JOIN_HOPPING_SEQUENCE`). In the default settings, it can take up to a few minutes.

On the other node, type `tsch-status` to check if the node has joined the network yet:
```
> tsch-status
TSCH status:
-- Is coordinator: 0
-- Is associated: 1
-- PAN ID: 0xabcd
-- Is PAN secured: 0
-- Join priority: 1
-- Time source: 0012.4b00.0616.0fcc
-- Last synchronized: 3 seconds ago
-- Drift w.r.t. coordinator: 4 ppm
```

Nodes should now be able to ping one another, as in [tutorial:ipv6-ping].  You can enable TSCH logs from the shell to find out more about how TSCH operates. When setting the log level to the maximum (`LOG_LEVEL_DBG`, or `4`), TSCH will output detailed per-log slots. These come from the `tsch-log.[ch]` module, which saves key information in every slot and dumps it over serial as soon as it can do so without interfering with TSCH timings. To interpret the logs, read [doc:tsch-logs]. To disable the logs at runtime, try `log mac 0` in the shell.

You can run TSCH transparently with RPL ([tutorial:rpl] and [tutotiral:rpl-border-router]). Note than whenever a node is set as RPL root, it will automatically become a TSCH coordinator.

## Enabling link-layer security

Our implementation supports standard link-layer security as per IEEE 802.15.4-2015 and 6TiSCH minimal configuration. To include security support in the firmware, add the following to your `project-conf.h`:
```c
#define LLSEC802154_CONF_ENABLED 1
```

When starting a node as coordinator via `tsch-set-coordinator`, use the second (optional) argument to enable/disable security:
```
> tsch-set-coordinator 1 1
Setting as TSCH coordinator (secured)
```

After joining, try `tsch-status` and check that the PAN is secured. This is an example output on a non-root node:
```
> tsch-status
TSCH status:
-- Is coordinator: 0
-- Is associated: 1
-- PAN ID: 0xabcd
-- Is PAN secured: 1
-- Join priority: 1
-- Time source: 0012.4b00.0616.0fcc
-- Last synchronized: 5 seconds ago
-- Drift w.r.t. coordinator: 0 ppm
```

With security enabled, all EBs are authenticated, and all data traffic is both encrypted and authenticated. To force a node to only join secure networks, set `TSCH_CONF_JOIN_SECURED_ONLY`.

[doc:tsch]: /doc/programming/TSCH-and-6TiSCH
[doc:tsch-logs]: /doc/programming/TSCH-and-6TiSCH
[tutorial:shell]: /doc/tutorials/Shell
[tutorial:logging]: /doc/tutorials/Logging
[tutorial:ipv6-ping]: /doc/tutorials/IPv6-ping
[tutorial:rpl]: /doc/tutorials/RPL
[tutotiral:rpl-border-router]: /doc/tutorials/RPL-border-router
[tutorial:ram-rom-usage]: /doc/tutorials/RAM-and-ROM-usage
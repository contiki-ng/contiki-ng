A RPL+TSCH node that demonstrates joining and the basic operation of TSCH and RPL networks.

Modes of operation
------------------

By default, the application is going to operate in the role of a regular RPL+TSCH node.
Change the value of `is_coordinator` variable at startup to put it in TSCH coordinator mode.
One Cooja mote and Zolertia Z1 platforms typically used for simulation, the coordinator
mode is automatically enabled if the node ID is equal to 1.

The coordinator mode can also be configured using the Contiki-NG shell.
Connect to the node via serial port and type this command in the shell:

    rpl-set-root 1

This is also going to automaticaly put it in TSCH coordinator mode.

There is also a command `tsch-set-coordinator`. However, setting the node in the TSCH
coordinator mode does NOT put the node in RPL root mode. Nodes that have joined
a TSCH network send out EB packets only if they also have joined a RPL DAG.
As a result, using this command is not recommended.

Command line settings
---------------------

The following command line options are available:
* `MAKE_WITH_ORCHESTRA` - use the Contiki-NG Orchestra scheduler.
* `MAKE_WITH_SECURITY` - enable link-layer security from the IEEE 802.15.4 standard.
* `MAKE_WITH_PERIODIC_ROUTES_PRINT` -  print routes periodically. Useful for testing and debugging.
* `MAKE_WITH_STORING_ROUTING` - use storing mode of the RPL routing protocol.
* `MAKE_WITH_LINK_BASED_ORCHESTRA` - use the link-based rule of the Orchestra shheduler. This requires that both Orchestra and storing mode routing are enabled.

Use the vaule 1 for "on", 0 for "off". By default all options are "off".

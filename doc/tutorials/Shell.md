# Shell

Contiki-NG provides a shell that enables interactive IPv6 host inspection and maintenance. 

To enable the shell in your project, simply add it to the module list (before the line `include $(CONTIKI)/Makefile.include`) in your Makefile:
```
MODULES += os/services/shell
```

As you have modified the Makefile, you need to clean:
```
$ make distclean
```

Now compile the firmware, flash a node, and connect to its serial interface (see [tutorial:hello-world]). Press enter to get a prompt:
```
0012.4b00.0616.0fcc>
```

If the shell is not responding to keystrokes, make sure your terminal emulator is sending the correct EOL character (Contiki-NG expects 0x0A (`\n`), whereas some terminal programs send 0x0D `\r`).

`help` will show you the list of available commands:
```
> help
Available commands:
'> help': Shows this help
'> reboot': Reboot the board by watchdog_reboot()
'> ip-addr': Shows all IPv6 addresses
'> ip-nbr': Shows all IPv6 neighbors
'> log module level': Sets log level (0--4) for a given module (or "all"). For module "mac", level 4 also enables per-slot logging.
'> ping addr': Pings the IPv6 address 'addr'
'> rpl-set-root 0/1 [prefix]': Sets node as root (1) or not (0). A /64 prefix can be optionally specified.
'> rpl-local-repair': Triggers a RPL local repair
'> rpl-refresh-routes': Refreshes all routes through a DTSN increment
'> rpl-global-repair': Triggers a RPL global repair
'> rpl-status': Shows a summary of the current RPL state
'> rpl-nbr': Shows the RPL neighbor table
'> routes': Shows the route entries
'> radio help': Shows radio command usage.
'> leds [on/off led]': Controls the leds.
```

Note that depending on your compile-time configuration, different commands might show. For instance, 6TiSCH will come with its own shell commands. With the `log` command, if you enable logging for some modules (see [tutorial:logging]), you will have the ability to change the log level at run-time on a per-module basis, with e.g. `>log rpl 2`.

Try `ip-addr`:
```
> ip-addr
Node IPv6 addresses:
-- fe80::212:4b00:616:fcc
```

Awesome, our node already has an IPv6 address! This is a link-local address (prefix `fe80`), auto-configured from the node's MAC address through EUI-64. We will get started with IPv6 communication in [tutorial:ipv6-ping].

## Troubleshooting: it doesn't work!!!
If you can see device output, but nothing happens after hitting the return key, the problem is most likely related to the End-of-Line character sent by your terminal emulator when you hit return. Contiki-NG's serial line code only interprets the `LF` char (`0x0A` / `\n`) as an EOL. Some terminal emulators send `CR` instead (`0x0D` / `\r`) and Contiki-NG's serial line code does not interpret this character as an EOL. This is a well-known problem with PuTTY.

* Windows:
  * PuTTY sends the `CR` char (`0x0D` / `\r`) when hitting the return key. At the time of writing this, there is no easy workaround.
  * Tera Term is an alternative FLOSS that is known to work at the time of writing this page.
* Linux:
  * GNU screen: This is untested at the time or writing this section, but `stty onlcr` should do the trick.
  * PuTTY's behaviour is the same as above.
* OSX: Coolterm sends `CR+LF` by default when hitting the return key (this is configurable), so the Contiki-NG shell should just work if you have done everything else correctly.

## Troubleshooting: it works, but I can't see what I'm typing

The Contiki-NG shell does not echo received keystrokes back. If you want to see what you are typing, as you are typing it, then enable the "local echo" option in your terminal emulator software. This is something that most (all?) terminal emulators support.

[tutorial:hello-world]: /doc/tutorials/Hello,-World!
[tutorial:logging]: /doc/tutorials/Logging
[tutorial:ipv6-ping]: /doc/tutorials/IPv6-ping

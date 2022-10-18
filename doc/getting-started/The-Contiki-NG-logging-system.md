# The Contiki‐NG logging system

## Using the logging system

Contiki-NG comes with a logging system that supports per-module log levels. It is implemented as `os/sys/log.c`, `os/sys/log.h` and `os/sys/log-conf.h`. The different log levels, for any given module, are:
* `LOG_LEVEL_NONE`: Logs disabled,
* `LOG_LEVEL_ERR`: Errors,
* `LOG_LEVEL_WARN`: Errors and warnings,
* `LOG_LEVEL_INFO`: Errors, warnings, and information logs,
* `LOG_LEVEL_DBG`: All the above and debug messages.

The file `log-conf.h` shows the different modules that currently support logging: `LOG_CONF_LEVEL_RPL` for RPL, etc. Log levels can be set from a `project-conf.h` file (see [doc:configuration]), with for instance `#define LOG_CONF_LEVEL_RPL LOG_LEVEL_WARN`. See `log-conf.h` for more logging options, such as compact address logging, inclusion of the line of code that issues the log, etc.

Finally, the log level can be changed at run-time, e.g. using the Shell ([tutorial:shell]). The log level for a given module can be set with `log_set_level`. Note that it is not possible to set at run-time a log level greater than what was compiled. Only lower log levels are possible. To enable any level at run-time, compile Contiki-NG with the maximum log level for all modules (this will however result in a larger ROM usage, and may not be practical on some platform).

## Supporting the logging system in a module

To support per-module logging, `.c` files that implement must set the following:
```c
#include "sys/log.h"
#define LOG_MODULE "Test"
#define LOG_LEVEL LOG_LEVEL_INFO
```

The first line includes the logging module. The second line defines the string used as a prefix for all logs of this module (use max 10 chars, for aligned output). The log level is then set to `LOG_LEVEL_INFO`. For examples of multi-file modules, see e.g. the RPL-Lite `.c` files.

To issue a log, use:
```c
LOG_ERR("some error code (%u)\n", 42);
LOG_WARN("some warning\n");
LOG_INFO("some information\n");
LOG_DBG("some debug message\n");
```

At run-time, when the log level is set to e.g. LOG_LEVEL_ERR, this results in:
```
[ ERR: Test] some error code (42)
```

To append a string to the current line, use the variant of the macros that are suffixed with `_`. For instance:
```c
LOG_ERR("some error message.. ");
LOG_ERR_("continued\n");
```

Will issue:
```
[ ERR: Test] some error message.. continued
```

Finally, the module provides macros suffixed with `_LLADDR` and `_6ADDR` for convenient logging of link-layer addresses and IPv6 addresses, respectively. Example usage:
```c
LOG_INFO("adding global IP address ");
LOG_INFO_6ADDR(&ipaddr);
LOG_INFO_("\n");
```

Note the final `LOG_INFO_("\n");`, needed to end the line as none of the `LOG_` macros add a `\n`. This example will issue, e.g.:
```
[INFO: Test] adding global IP address fd00::aaaa:bbbb:cccc:dddd
```
or, with `LOG_CONF_WITH_COMPACT_ADDR` enabled:
```
[INFO: Test] adding global IP address 6G-dddd
```

[doc:configuration]: /doc/getting-started/The-Contiki-NG-configuration-system
[tutorial:shell]: /doc/tutorials/Shell

# native: Contiki-NG as a native process

This platform enables running Contiki-NG as a native process, that is, a Linux, OSX or Windows executable.
The serial port is then connected to stdin and stdout.
Add the shell [tutorial:shell] to be able to interact with the process.

If the process is started with sufficient permissions, a tun interface will connect the Contiki-NG stack to the host OS.
The IPv6 ping example demonstrates this feature in [tutorial:ping].

[tutorial:shell]:/doc/tutorials/Shell
[tutorial:ping]:/doc/tutorials/IPv6-ping

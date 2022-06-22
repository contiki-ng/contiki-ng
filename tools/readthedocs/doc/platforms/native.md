This platform enables running Contiki-NG as a native process, that is, a Linux, OSX or Windows.
The serial port is then connected to stdin and stdout.
Add the shell [tutorial:shell] to be able to interact with the process.

If the process is started with sufficient permissions, a tun interface will connect the Contiki-NG stack to the host OS.
The IPv6 ping example demonstrates this feature in [tutorial:ping].

[tutorial:shell]:https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-Shell
[tutorial:ping]:https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-IPv6-ping
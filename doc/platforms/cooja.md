# cooja: Cooja native motes platform

This platform is a virtual platform, used by the Cooja platform to run Contiki-NG as 'Cooja motes'.
It compiles Contiki-NG as a native process, and connects directly all hardware accesses to the Cooja simulator.
Unlike emulations, simulation with Cooja motes are not perfectly time-accurate, but the timing is still good enough to run the MAC layers CSMA and TSCH.

From within a container, start cooja with `cooja`, or from your host, spawn Cooja inside a new container with `contiker cooja`.

A Cooja tutorials where one can try Cooja motes is available at: [tutorial:cooja].

[tutorial:cooja]: /doc/tutorials/Running-Contiki-NG-in-Cooja
[doc:docker]: /doc/getting-started/Docker

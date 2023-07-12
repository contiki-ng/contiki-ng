# Repository structure

The Contiki-NG repository is structured as follows:
* `os`: Contains the actual Contiki-NG code. Includes the systems primitives such as processes and timers, the networking stack, and all libraries and services. All examples also compile and link to `os`. See [doc:contiki-ng] for more information.
* `arch`: Contains all hardware-dependent code. This includes CPU, device and platform drivers. A list of supported platforms can be found under `arch/platforms` and its sub-directories. This is where to put your code if you are porting Contiki-NG to your own platform. Current platforms are documented at [doc:platforms].
* `examples`: Contains ready-to-use example projects. Shows how to use networking, libraries and storage services. Includes a RPL border router, the slip-radio interface etc. To write your own application, start from one of the examples and follow our tutorials at [doc:tutorials].
* `tools`: Contains tools, which are not to be included in a Contiki-NG firmware, but rather intended to run on a computer. Includes flashing tools, the Cooja simulator (as a submodule), Docker and Vagrant scripts, and more.
* `tests`: Contains all continuous integration tests run for every pull request and merge, to ensure non-regression.

[doc:contiki-ng]: /index.rst
[doc:platforms]: /doc/platforms/index.rst
[doc:tutorials]: /doc/tutorials/index.rst

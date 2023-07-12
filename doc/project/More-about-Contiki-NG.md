# More about Contiki‐NG

In 2017, Contiki-NG started as a fork of the Contiki operating system with the following goals:
* Focus on dependable (reliable and secure), standard-based IPv6 communication;
* Focus on modern IoT platforms, e.g. ARM Cortex M3 and other 32-bit MCUs;
* Modernize the structure, configuration, logging and platforms, to reflect the goals above;
* Improve the documentation, both code API, module description, and tutorials;
* Implement a more agile development process, with easier inclusion of new features, and with periodic releases.

The first version is Contiki-NG 4.0.
A detailed changelog is available at [doc:releases].

## Getting Around
If you are coming from Contiki, this is what you need to know:
* Documentation and tutorials are available at [doc:home]. We still use Doxygen, but most of the content has moved to the wiki. Only code APIs are left as doxygen documentation, available at [doxygen]
* The former directory `core` is renamed `os`. `apps` are moved to `os`. The top-level `dev`, `cpu` and `platform` are under a new top-level directory `arch`.
* Examples are still under the top-level `examples` directory
* The configuration system, in particular how the network stack is set up, has changed significantly. Read [doc:configuration-system]
* The build system is overall unchanged, but offers a number of new handy commands (configuration inspection, intermediate build file generation). It is documented at [doc:build-system]

[doxygen]: https://contiki-ng.readthedocs.io/en/develop/_api/index.html
[doc:home]: https://docs.contiki-ng.org/
[doc:configuration-system]: /doc/getting-started/The-Contiki-NG-configuration-system.md
[doc:build-system]: /doc/getting-started/The-Contiki-NG-build-system.md
[doc:releases]: https://github.com/contiki-ng/contiki-ng/releases

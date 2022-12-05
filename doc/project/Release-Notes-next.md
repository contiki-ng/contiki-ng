# Press release

We are releasing version XXX of Contiki-NG. This release adds XXX, and support
for link-time-optimization that can reduce the binary size up to 50% on some
examples.

Various parts of the user experience for developers have been improved in this release:
* Docker image updated to Ubuntu 22.04.

## Find out more at:

* GitHub repository: https://github.com/contiki-ng/contiki-ng
* Documentation: https://contiki-ng.readthedocs.io/en/release-v4.9
* Web site: http://contiki-ng.org
* Nightly testbed runs: https://contiki-ng.github.io/testbed

## Engage with the community:

* Contiki-NG tag on Stack Overflow: https://stackoverflow.com/questions/tagged/contiki-ng
* Github Discussions: https://github.com/contiki-ng/contiki-ng/discussions
* Gitter: https://gitter.im/contiki-ng
* Twitter: https://twitter.com/contiki_ng

The Contiki-NG team

## API changes for ports outside the main tree

## Cooja API changes for plugins outside the main tree

### Update from JDOM 1 to JDOM 2

JDOM was upgraded from version 1 to version 2 ([#784](https://github.com/contiki-ng/cooja/pull/784)).
This requires some source code updates, but since Cooja uses such a small subset
of the JDOM API, the update can be done automatically with the command:

```bash
find <directory> -name \*.java -exec perl -pi -e 's#import org.jdom.#import org.jdom2.#g' {} \;
```

### Avoid starting the AWT thread in headless mode

Cooja will no longer start plugins that extend `VisPlugin` in headless mode
to avoid starting the AWT thread. Plugins that should run in both GUI mode
and headless mode need to be updated to keep the JInternalFrame internal.
Examples for PowerTracker and other plugins can be found in the PR
([#261](https://github.com/contiki-ng/cooja/pull/261)).

## Changelog

### Contiki-NG

* Support for link-time-optimization in the build system ([#2077](https://github.com/contiki-ng/contiki-ng/pull/2077))

All [commits](https://github.com/contiki-ng/contiki-ng/compare/release/v4.8...develop) since v4.8.

### Cooja

* Mobility plugin added to Cooja ([#768](https://github.com/contiki-ng/cooja/pull/768))

All [commits](https://github.com/contiki-ng/cooja/compare/630e719d01d3...master) since v4.8.

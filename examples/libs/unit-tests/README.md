# Example unittest project
This example shows how to build unit tests project. Structure is similar to
test suites in `tests/` directory.

Test suite is a normal Contiki-NG-based mote that defines and run unit tests.
The Contiki project is in `code-unittests`. Tests use the `os/services/unit-test`
library; main process run every test. Results are printed on standard output.

Test is compiled and run by Cooja with script from `js/ut-runner.js`.
You can open the `00-unit-tests.csc` simulation and run it from GUI, or by
command, e.g. in _contiker_ container:

    make -C examples/libs/unit-tests/

In similar way you can add complex Cooja simulations as a test.

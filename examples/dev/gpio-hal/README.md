# dev/gpio-hal

## GPIO HAL Example
This example demonstrates and tests the functionality of the GPIO HAL. You can
use it to:

* Understand the logic of the GPIO HAL.
* Test your implementation of arch-specific GPIO HAL components if you are
developing a new port.

This example assumes a device with:

* 3 output pins (e.g. LEDs). If the platform uses the port/pin GPIO numbering
  scheme then this example assumes that at least 2 of those pins are part of
  the same port.
* 1 button.

### Supported devices
This example is expected to work off-the-shelf on the following boards:

* All CC13xx/CC26xx devices
* All CC2538 devices
* nRF52840
* Cooja motes

### Extending for other platforms
Create a sub-directory with the same name as your platform. For example, for
platform `my-new-platform` create a subdirectory called `my-new-platform`.
Source files in this directory will be compiled automatically. In the most
simple case, all you will need is a source file called e.g. `pins.c` (it's OK
to use a different filename). In this file, you will need to provide
definitions of the variables used by the example to manipulate pins. These
variables are:

* `out_pin1`, `out_pin2` and `out_pin3` for output pins.
* `btn_pin` for the button pin.

Assign to those variables a value that corresponds to the output pin in your
board that you wish to test with the example. For example, if you have a LED
connected to pin 20, then you will need to

    gpio_hal_pin_t out_pin1 = 20;

If the platform uses the port/pin GPIO numbering scheme then you will also
need to provide definitions for the following variables:

* `out_port1` and `out_port2_3`. `out_port2_3` corresponds to the port that
  `out_pin2` and `out_pin3` are members of.
* `btn_port` for the button pin's port.

# platform-specific/cc26x0-cc13x0/base-demo

## CC13xx/CC26xx Base Demo

This example demonstrates basic functionality for the various CC13xx/CC26xx
boards supported by Contiki-NG.

This example is intended to work for both the older `cc26x0-cc13x0` target, as
well as for the newer `simplelink` target.

However, this example will _NOT_ work for the sensortag board when using the
`simplelink` target. This functionality will be added in the future.

The example demonstrates:

* How to take sensor readings
* How to use buttons and the reed relay (triggered by holding a magnet near S3
  on the SensorTag).
* How to send out BLE advertisements, if the chip has BLE capability. The
  device will periodically send out BLE beacons with the platform name as
  payload. Those beacons/BLE ADV packets can be captured with any BLE-capable
  device. Two such applications for iOS are the TI Multitool and the TI
  Sensortag app. They can be found in the Apple App Store. If you have a
  BLE-capable Mac, you can also use LightBlue for OS X.

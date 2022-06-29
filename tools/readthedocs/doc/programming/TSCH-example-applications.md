# TSCH example applications

The TSCH and 6TiSCH example applications can be found under `examples/6tisch/`.

They are:
* `simple-node` - a RPL+TSCH node that demonstrates joining and the basic operation of TSCH and RPL networks. Should be a starting point when learning TSCH and 6TiSCH with Contiki-NG
* `timesync-demo` - a demonstration of application-level time synchronization using TSCH.
* `tsch-stats` - a demo of the TSCH statistics module which collects per-channel packet acknowledgment rates and background noise RSSI.
* `custom-schedule` - an example that shows how to manually add cells in the TSCH schedule. Useful for those who want to beyond the default Contiki-NG Orchestra schedule.
* `channel-selection-demo` - demonstration of TSCH adaptive channel selection based on background noise RSSI metric. This is a non-standard extension that can be enabled in Contiki-NG for increased reliability.
* `sixtop` - a demo of the 6top distributed scheduling protocol. This does not feature the 6TiSCH Minimal Scheduling Function, just a simple custom scheduling function on top of the 6top protocol.


There are also some test applications:
* `etsi-plugtest-2017` - interoperability testing from the ETSI plug-test event in 2017.
* `6p-packet` - 6top protocol packet demo/test.

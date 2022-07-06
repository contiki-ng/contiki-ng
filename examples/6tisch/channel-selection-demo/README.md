# 6tisch/channel-selection-demo

Demonstration of TSCH adaptive channel selection based on background noise RSSI metric.
TSCH stats must be enabled for the adaptive selection functionality to compile and work.

This code relies on the `os/services/channel-selection` library that implements
the "RSSI upstream" adaptative channel selection strategy, described in the following paper:

A. Elsts, X. Fafoutis, G. Oikonomou and R. Piechocki. Adaptive Channel Selection in IEEE 802.15.4 TSCH Networks, 1st Global Internet of Things Summit, 2017.
http://ieeexplore.ieee.org/document/8016246/
This example is intended to run on a mode that is connected to a native host
system by SLIP. Full IP packets are transmitted over slip, and a mote adds
IEEE 802.15.4 frame header. The mote has fully functional MAC and radio driver,
and the RPL and 6LoWPAN stack running on the host. This is typically used
with the native border router (example `rpl-border-router` on target native).

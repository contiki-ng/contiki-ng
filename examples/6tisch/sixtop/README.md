6top Example Description
------------------------

By default, the code will operate in the role of a regular RPL+TSCH node.
Change the value of `is_coordinator` variable at startup to put it in TSCH coordinator mode.

6top Operation
---------------

If the mode is not in the TSCH coordinator mode:

* The application triggers a 6P Add Request to 6dr (neighbor)
* Following this the application triggers another 6P Add Request to 6dr
* After an interval, the application triggers a 6P Delete Request to 6dr

For the Cooja simulation, you may use the rpl-tsch-sixtop-cooja.csc file in this folder.
Once you run the simulation, "Mote output" window of Cooja simulator displays the
following messages.

For a 6P Add transaction,
		ID:1 TSCH-sixtop: Sixtop IE received
		ID:1 TSCH-sixtop: Send Link Response to node 2
		ID:2 TSCH-sixtop: Sixtop IE received
		ID:2 TSCH-sixtop: Schedule link x as RX with node 2
		ID:2 TSCH-sixtop: Schedule link x as TX with node 1

Similarly for a 6P Delete transaction.

README file for CC2538 common examples
=========

The `cc2538-commmon` groups examples common to all cc2538-based platforms.
The examples in this folder are known to work on:

* cc2538dk (default)
* oppila
* zoul

To change the default target at compilation time you can add the following:

make TARGET=zoul

make TARGET=oppila

Or to define the default platform permanently:

make TARGET=zoul savetarget

make TARGET=oppila savetarget

This will create a `Makefile.target` file with the TARGET predefined.

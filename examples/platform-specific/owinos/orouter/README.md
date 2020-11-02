OMote IP64 README file
========================

This example shows how to use the Owinos Orouter Ethernet connectivity, based on the cc2538 and ENC28J60 modules.

IP64 router
-----------------
The router packs a built-in webserver and can run on 2.4GHz radio interface.

````
#define NETSTACK_CONF_RADIO         cc2538_rf_driver
#define ANTENNA_SELECT_DEF_CONF  ANTENNA_SELECT_2_4GHZ
````
To compile and flash run:

````
cd ip64-router
make TARGET=owinos BOARD=orouter ip64-router.upload
````

As default we enable the `DHCP` support for autoconfiguration.  Just connect to a DHCP-enabled device to obtain an IPv4 IP address and that's it!.

HTTP client examples
-----------------

There are available 2 examples ready to use using the `http-socket` library:

* The `client` example just makes a HTTP `GET` request to a know page and retrieves
  the result.

* The `ifttt-client` example sends a HTTP `POST` request to [IFTTT](https://ifttt.com/recipes) whenever the user button is pressed, building an Internet button to connect to several channels and applications, such as `Drive`, `Evernote` and many others.

To configure the `IFTTT` demo just edit the `project-conf.h` file and change the name of the event and write your API key:

````
#define IFTTT_EVENT   "button"
#define IFTTT_KEY     "XXXXXX"
````

To compile and flash:

````
cd client
make TARGET=owinos ifttt-client.upload
````


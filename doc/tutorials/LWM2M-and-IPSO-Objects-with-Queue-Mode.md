# LWM2M and IPSO Objects with Queue Mode

This tutorial assumes you have first been through the main LWM2M tutorial [tutorial:lwm2m].

## LWM2M with Queue Mode
Queue Mode is an energy saving strategy that LWM2M contains in its specification. It is based on allowing the client to sleep for long periods of time (the **sleep time**), where the server cannot make any requests. When the client wakes up, it sends a registration update message to inform the server, and it can receive requests. When a well-defined time has passed after the last received request (the **awake time**), it enters sleep mode again.

Queue Mode is supported in the LWM2M's implementation. To enable it, the next flag should be enabled in the project-conf.h: 
```c
#define LWM2M_QUEUE_MODE_CONF_ENABLED 1`
```
This will use the default awake and sleep times defined in lwm2m-queue-mode-conf.h, which are 5 and 10 seconds. To change this values, add the following to the project-conf.h:
```c
#define LWM2M_QUEUE_MODE_CONF_DEFAULT_CLIENT_AWAKE_TIME desired_time_in_ms
#define LWM2M_QUEUE_MODE_CONF_DEFAULT_CLIENT_SLEEP_TIME desired_time_in_ms
```
Apart from selecting fixed times, the dynamic adaptation for the client awake time can be enabled. This will optimize the awake time in order to save more energy. It is based on measuring the time between every two consecutive requests from the server, and adapt the awake time by taking the maximum measured time and adding a safety margin. With this, the client does not stay awake for more time than needed. To enable it, add in project-conf.h:
```c
#define LWM2M_QUEUE_MODE_CONF_INCLUDE_DYNAMIC_ADAPTATION 1
#define LWM2M_QUEUE_MODE_CONF_DEFAULT_DYNAMIC_ADAPTATION_FLAG 1
```
After that, the Queue Mode object can be added in the client's resource directory. This is an object with the ID 30000 that contains three different resources: the awake time, the sleep time and the dynamic adaptation flag. The server can write in these resources to change the values dynamically, or read them. To add the Queue Mode object, include in the project-conf.h:
```c
#define LWM2M_QUEUE_MODE_OBJECT_CONF_ENABLED 1
```
Finally, the sleep behavior needs to be defined. This is platform-dependent and the user needs to define these actions for each platform where Queue Mode is used (only the zoul platform is provided now). To control this behavior, two macros are used: 

* `LWM2M_Q_MODE_WAKE_UP`: actions to perform when the client wakes up, like turning on the radio.
* `LWM2M_Q_MODE_SLEEP_MS(TIME_MS)`: actions to perform to put the client into sleep mode, like turning off the radio.

In the case of the example, these macros are defined for the zoul platform in the file `zoul/module-macros.h`. For this platform, the actions to sleep are to turn off the radio and start a sleep timer in order to wake up after some time. With this timer set, Contiki-NG will drive the CPU into low power mode. The actions to wake up are to turn on the radio again. So the macros look like this:
```c
#define LWM2M_Q_MODE_WAKE_UP()  do { \
	NETSTACK_MAC.on();	\
} while(0)

#define LWM2M_Q_MODE_SLEEP_MS(TIME_MS)  do { \
	uint64_t aux = TIME_MS * RTIMER_SECOND;	\
	NETSTACK_MAC.off();	\
	rtimer_arch_schedule(RTIMER_NOW() + (rtimer_clock_t)(aux / 1000));	\
} while(0)
```

After doing all this, the `example-ipso-objects.c` can be compiled and programmed into the device as it was explained in [tutorial:lwm2m]. For the Queue Mode usage, the border router needs to be set up too. Using it, the device will try to register to the server using Queue Mode as binding mode. 

The last part is to set up the Leshan server to run with Queue Mode support. For doing that, run:
```bash
$ wget https://carlosgp143.github.io/resources/leshan-server-demo-1.0.0-qmode-tutorial.jar
$ java -jar leshan-server-demo-1.0.0-qmode-tutorial.jar
```
This is an extension of the Leshan's Queue Mode implementation with a queue to store the requests that are generated when the client is sleeping. If the user clicks the request buttons in the web application when the client is sleeping, they are placed in the queue and send when the client is awake again (informed with an update message). Also, the server prints the following when the state of the client changes to awake/sleeping:
```bash
Queue Mode Listener: client: "client_name" is awake at "hour:minutes:seconds"
Queue Mode Listener: client: "client_name" is sleeping at "hour:minutes:seconds"
```  
To change the state from awake to sleeping, the Leshan server needs to know the client's awake time, since the client does not send any message before going to sleep. In order to do that, it makes a GET request to the awake time resource present in the Queue Mode object directly after the the update message. This can be seen in the print log too:
```
Client awake time read: "time_in_ms"
```
Once everything is set up, the user can change the awake/sleep times through the web application in `localhost:8080` and see that they are actually applied in the client's behavior.

[tutorial:lwm2m]: /doc/tutorials/LWM2M-and-IPSO-Objects

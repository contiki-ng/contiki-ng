# lwm2m-ipso-objects

## LWM2M with IPSO Objects Example

This is an OMA LWM2M example implementing IPSO Objects.
It can connect to a Leshan server out-of-the-box.
Important configuration parameters:
* `LWM2M_SERVER_ADDRESS`: the address of the server to register to (or bootstrap from)
* `REGISTER_WITH_LWM2M_BOOTSTRAP_SERVER`: set to bootstrap via `LWM2M_SERVER_ADDRESS` and then obtain the registration server address

A tutorial for setting up this example is provided on the wiki.

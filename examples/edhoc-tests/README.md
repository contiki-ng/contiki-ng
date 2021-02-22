An EDHOC Client and Server that demonstrate the EDHOC module based on IETF[draft-ietf-lake-edhoc-02] draft, running as RPL node and RPL border router respectively on both Zolertia REMote hardware platform and Cooja
Simulator(`edhoc-tests-cooja.css `).

#EDHOC Client Example
An EDHOC Client Example is provided at `examples/edhoc-tests/edhoc-test-client.c `.
For the specific example the EDHOC Server IP must be selected on the project-conf file, its own Node Key Identity and, the EDHOC part as Initiator:

```c
#define EDHOC_CONF_SERVER_EP "coap://[fd01::201:1:1:1]" /* Server IP for Cooja simulator */

#define EDHOC_CONF_PART PART_I
```

Additionally, the node runs with RPL by:
```c
#define EDHOC_CONF_RPL_NODE 1
```

#EDHOC Server Example
An EDHOC Server Example is provided at `examples/edhoc-tests/edhoc-test-server.c ` together with the corresponding EDHOC plug test resource at
`examples/edhoc-tests/res-edhoc.c `.The specific example runs the EDHOC Responder protocol part on the CoAP server at the Border Router.Can run on
constrained device or natively at a host.

The Server Identity must be selected at:

```c
#define AUTH_SUBJECT_NAME "Server_key_identity"
```

When the example runs on Cooja simulator the following cmd must be executed at the linux side of the border router:
- `tunslip6-a 127.0.0.1 - p 60001 fd01::1/64` runs
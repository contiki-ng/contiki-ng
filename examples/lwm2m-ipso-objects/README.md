# lwm2m-ipso-objects

## LWM2M with IPSO Objects Example

This is an OMA LWM2M example implementing IPSO Objects.
It can connect to a Leshan server out-of-the-box.
Important configuration parameters:
* `LWM2M_SERVER_ADDRESS`: the address of the server to register to (or bootstrap from)
* `REGISTER_WITH_LWM2M_BOOTSTRAP_SERVER`: set to bootstrap via `LWM2M_SERVER_ADDRESS` and then obtain the registration server address

A tutorial for setting up this example is provided on the wiki.

Notes for using DTLS (Mbed TLS implementation) with LwM2M:
* DTLS implementation notes are provided in `os/net/app-layer/coap/README.md`.
* Leshan has a default re-transmission timeout of 2s. However this might be insufficient when an embedded platform is used on the client-side with an ECC-based ciphersuite. ECC operations usually take a long time during the HS (e.g., ~5s for nRF52840 platform). Hence, the re-transmission timeout on the Leshan side must be increased accordingly. 
* Steps to generate certificates and to plug them into Leshan is provided on the Leshan wiki/Credential-files-format. They can be provided in PEM format to the DTLS module in Contiki-NG as strings. 
* A set of test certificates are generated and placed in project-conf-dtls.h as strings. To use these test credentials, compile with `MAKE_COAP_DTLS_KEYSTORE=MAKE_COAP_DTLS_KEYSTORE_SIMPLE`. However, note that these certificates contain a Common Name (CN) `Contiki-NG36065E1DD`, which is the device used for testing. Using these certificates will fail the certificate parsing step. 
* Tested with Leshan version 2.0.0

# Ephemeral Diffie-Hellman Over COSE (EDHOC) [draft-ietf-lake-edhoc-01]

The [draft-ietf-lake-edhoc-01] IETF Internet-Draft specifies Ephemeral Diffie-Hellman Over COSE (EDHOC), a very compact, and lightweight authenticated Diffie-Hellman key exchange with ephemeral keys that provides mutual authentication, perfect forward secrecy, and identity protection. It uses COSE [RFC8152] for
cryptography, CBOR [RFC7049] for encoding, and CoAP [RFC7252] for transport and the main use case is to establish an OSCORE security context.  
The EDHOC exchange and the key derivation follow known protocol constructions such as [SIGMA], NIST SP-800-56A and HKDF [RFC5869].

## EDHOC in Contiki-NG
The Contik-NG EDHOC module implememts asymmetric key authentication by using static Diffie-Hellman keys. The authentication is provided by a Message Authentication Code (MAC) computed from an ephemeral-static ECDH shared secret which enables significant reductions in message sizes. The implementation uses cypher suite 2 that consists of AES-CCM-16-64-128 as an AEAD algorithm, SHA-256 as a Hash algorithm, and the P-256 as ECDH curve. 

EDHOC consists of three messages (MSG1,MSG2 and, MSG3), plus an EDHOC error message (MSG_ERR) where each of them is a CBOR sequence. The current implementation transports these messages as an exchange of Confirmable CoAP [RFC725] messages where the CoAP client is the EDHOC Initiator and the 
CoAP server is the EDHOC Responder. The MSG1 and MSG3 are transferred in POST requests and MSG2 in a 2.04 (Changed) response to the Uri-Path: "/.well-known/edhoc". When MSGs size is bigger than 64B the Block-Wise transfer mechanism [RFC7959] for fragmentation is being used.

Notice that the authentication keys must be established at the EDHOC key storage before running the EDHOC protocol. For this reason, an edhoc-key-sotrage.h() API function is provided in order to set the COSE_key with the correct struct format.
At the configuration file, the credential type used for authentication must be selected. Two methods have been implemented:
- `PRK_ID`: The EDHOC exchanging a unique identity of the public authentication key to be retrieved.Before running the EDHOC protocol each party need at least a DH-static public key and a set of identities which is allowed to communicate with.
- `PRKI`: The EDHOC exchanging messages which include directly the actual credential (DH-static public compressed key). The EDHOC protocol can run without prior knowledge of the other part. Each part provisionally accepts the RPK of the other part until posterior authentication.

## EDHOC dependencies
The EDHOC module implementation depends on the following libraries:

### Micro-ECC
A small and fast ECDH and ECDSA implementation for 8-bit, 32-bit, and 64-bit processors. Implements five standard NIST curves, with `secp256r1` among them. The library is implemented in C and can be optionally optimised for either speed or code size at compilation time. 
The specific external repository is added as a submodule in `os/net/security/uecc` folder.
- Author: K. Mackay
- Link: [MicroECC](https://github.com/kmackay/micro-ecc/tree/601bd11062c551b108adbb43ba99f199b840777c)

The EDHOC module uses the `uecc` submodule to generate the ephemeral Diffie-Hellman pairs key and the EDHOC shared secrets.

### SH2 (Secure Hash Algorithm) 
The EDHOC module use SHA-256 (Secure Hash Algorithm) in HMAC-SHA256 (Hash-based message authentication) for key derivation functionalities in order to compute the transcript-hash (TH). The `EDHOC_CONFIG_SHA` macro can be defined at the config file to choose between the following SH256 libraries: 
- SHA HW for CC2538 SH2 modules: Contiki-ND driver for the cc2538 SHA-256 mode of the security core.
- SHA SW library from Oriol Pinol [sha](https://github.com/oriolpinol/contiki/tree/master/apps/ecc): The SHA-256 is included under `os/net/security/sha` folder.
 

### CBOR (Concise Binary Object Representation)
The EDHOC module uses CBOR to encode the EDHOC exchanging messages. The `cbor` Contiki-NG implementation has been added from `group-oscore` in `lib/cbor` folder. 
- Author: Martin Gunnarsson
- Link: [group-oscore](https://github.com/Gunzter/contiki-ng/tree/group_oscore/os/net/app-layer/coap/oscore-support)  

A `cbor_put_bytes_identifier()` function has been added in order to cover the EDHOC protocol requirements. 

### COSE module
The EDHOC module uses the COSE_Encrypt0 struct encryption from [RFC8152] for cryptography as well as the COSE_key format. The required COSE functionality has been implemented in a lightweight module under `os/net/security/cose`.

## EDHOC Client API
The edhoc-client-API.h file contains the EDHOC interface to be used by the EDHOC Initiator part as CoAP client. 

- `edhoc_client_run()`: Runs the EDHOC Initiator part. This function must be called from the EDHOC Initiator program to start the EDHOC protocol as Initiator. Runs a new process that implements all the EDHOC protocol and exits when the EDHOC protocol finishes successfully or the EDHOC_CONF_ATTEMPTS are exceeded. 
- `edhoc_client_callback()`: This function checks the events trigger from the EDHOC client process and returns '1' when the EDHOC protocol successfuly ends, -1 when the EDHOC protocol max retries are exceeded, and 0 when the EDHOC client process is steel running.
- `edhoc_client_close()`: This function must be called after the Security Context is exported to free the allocated memory. 
- Optionally some functionalities for getting and setting the Aditional Application Data of the EDHOC messages are provided. 

Once the EDHOC Client process is successfully finished,  the security context can be exported by using the ehdoc-exporter.h API. For example:
- `edhoc_exporter_oscore.h()`: This function is used to derive an OSCORE Security Context [RFC8613] from the EDHOC shared secret.

### EDHOC Client Example
An EDHOC Client Example is provided at `examples/edhoc-tests/edhoc-test-client.c`.
For the specific example the EDHOC Server IP must be selected on the project-conf file, its own Node Key Identity and, the EDHOC part as Initiator:

```c
#define EDHOC_CONF_SERVER_EP "coap://[fd01::201:1:1:1]" // Server IP for works in Cooja simulator

#define AUTH_KEY_IDENTITY "Node_Key_Identity"

#define EDHOC_CONF_PART PART_I
```

Additionally, the node runs with RPL by:
```c
#define EDHOC_CONF_RPL_NODE 1
``` 
Also,  the number of attempts and the timeout can be set by:
```c  
#define EDHOC_CONF_ATTEMPTS 3

#define EDHOC_CONF_TIMEOUT 10000
```

## EDHOC Server API
The edhoc-server-API.h file contains the EDHOC interface to be used by the EDHOC Responder as CoAP Server

- `edhoc_server_resource()`: This function must be called from the EDHOC plug test resource at a CoAP response POST handler to run the EDHOC protocol Responder part.
- `edhoc_server_init()`: This function activates the EDHOC well-known CoAP Resource (that runs edhoc_server_resource() functionality) at the uri-path defined on the WELL_KNOWN macro (`./well-known.edhoc` by default).
- `edhoc_server_start()`: This function gets the DH-static authentication pair keys of the Server by using the edhoc-key-storage API, creates a new EDHOC context and generates the DH-ephemeral key for the specific session. A new EDHOC protocol session must be created for each new EDHOC client transaction.
- `edhoc_server_close.h()`: This function must be called after the Security Context is exported to free the allocated memory. 
- Optionally some functionalities for getting and setting the Aditional Application Data of the EDHOC messages are provided.

From every client that the EDHOC server side is successfully finished, the security context to be used with the specific client can be exported by using the ehdoc-exporter.h API. For example:
- `edhoc_exporter_oscore.h()`: This function is used to derive an OSCORE Security Context [RFC8613] from the EDHOC shared secret.

### EDHOC Server Example

An EDHOC Server Example is provided at `examples/edhoc-tests/edhoc-test-server.c` together with the corresponding EDHOC plug test resource at
`examples/edhoc-tests/res-edhoc.c`. The specific example runs the EDHOC Responder protocol part on the CoAP server at the Border Router. Can run on
constrained device or natively at a host. 

The Server Identity must be selected at:

```c
#define AUTH_KEY_IDENTITY "Server_key_identity"
```

## EDHOC Tests
The EDHOC implementation has been tested running both `example\edhoc-test\edhoc-test-client.c` and `example\edhoc-test\edhoc-test-server.c` on Cooja and Zolertia RE-Mote platforms. In the latter case, it is mandatory to either set the 32Mhz Clock or else disable the watchdog so that the [MicroECC] library runs without modifications, by including one of the following definitions. 
```c
#define SYS_CTRL_CONF_SYS_DIV SYS_CTRL_CLOCK_CTRL_SYS_DIV_32MHZ  

#define WATCHDOG_CONF_ENABLE 0x00000000
```
 
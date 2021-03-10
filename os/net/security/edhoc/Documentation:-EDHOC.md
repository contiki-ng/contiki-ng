#Ephemeral Diffie-Hellman Over COSE (EDHOC) [draft-ietf-lake-edhoc-05]

The [draft-ietf-lake-edhoc-05] IETF Internet - Draft specifies Ephemeral Diffie-Hellman Over COSE (EDHOC), a very compact, and lightweight authenticated Diffie-Hellman key exchange with ephemeral keys that provides mutual authentication, perfect forward secrecy, and identity protection.It uses COSE [RFC8152] for cryptography, CBOR [RFC7049] for encoding, and CoAP [RFC7252] for transport and the main use case is to establish an OSCORE security context. The EDHOC exchange and the key derivation follow known protocol constructions such as [SIGMA], NISTSP-800-56A and HKDF [RFC5869].

## EDHOC in Contiki-NG
The Contik-NG EDHOC module implememts asymmetric key authentication by using static Diffie-Hellman keys.The authentication is provided by a Message Authentication Code (MAC) computed from an ephemeral-static ECDH shared secret which enables significant reductions in message sizes. 
The implementation uses cypher suite 2 that consists of AES-CCM-16-64-128 as an AEAD algorithm, SHA-256 as a Hash algorithm, and the P-256 as ECDH curve.
The implementation has passed the interoperability test succesffuly in the IETF-Hacktachon 110.

EDHOC consists of three messages (MSG1, MSG2 and, MSG3), plus an EDHOC error message (MSG_ERR) where each of them is a CBOR sequence.The current implementation transports these messages as an exchange of Confirmable CoAP [RFC725] messages where the CoAP client is the EDHOC Initiator and the
CoAP server is the EDHOC Responder.The MSG1 and MSG3 are transferred in POST requests and MSG2 in a 2.04(Changed) response to the Uri-Path: 
"/.well-known/edhoc".When MSGs size is bigger than 64B the Block-Wise transfer mechanism [RFC7959] for fragmentation is being used.

Notice that the authentication keys must be established at the EDHOC key storage before running the EDHOC protocol.For this reason, an edhoc-key-sotrage.h() API function is provided in order to set the COSE_key with the correct struct format.
At the configuration file, the credential type used for authentication must be selected.Two methods have been implemented:
- `PRK_ID` : The EDHOC exchanging a unique identity of the public authentication key to be retrieved.Before running the EDHOC protocol each party need at least a DH-static public key and a set of identities which is allowed to communicate with.
- `PRKI_2` : The EDHOC exchanging messages which include directly the actual
credential (DH-static public key) formatted as a COSE_Key of type EC2. The EDHOC protocol can runs without prior knowledge of the other part.Each part provisionally accepts the RPK of the other part until posterior authentication.

### EDHOC configuration
The following macro must be definded on the configuration file:
- Define the KID of the authentication key used on this node.
```c
#define AUTH_KID KID
```
If the parties have agreed on an identity beside the public key, the "subject name" can be definded instead.
```c
#define AUTH_SUBJECT_NAME "Node_Key_Identity"
```
- Define the part roll taking on the EDHOC pprotocol.`PART_I` for the Initiator and `PART_R` for the Responder.
```c
#define EDHOC_CONF_PART PART_I
```
- Define the Conexion Identifier(`CID`)
```c
#define EDHOC_CID 0x20
```
- Define mechanism for correlating messages.When CoAP is used, as in this implementation, with the client as Initiator the trasport provide the responder to correlate message_2 and message_1 and `EXTERNAL_CORR_U` macro must be definded. But the EDHOC implementation include also the other three correlation mechanims definded on the protocol.
```c
#define EDHOC_CONF_CORR EXTERNAL_CORR_U
```
- Define which library to use for ECDH operations.The SW library of `micro-ECC` with the macro `UECC_ECC` or the HW driver acelerator for cc2538 modules with the macro `CC2538_ECC .
```c
#define EDHOC_CONF_ECC CC2538_ECC
```
- Define which library to use for SHA operations.The SW library of Pinol with the macro `DECC_SH2` or the HW driver acelerator for cc2538 modules with the macro ` CC2538_SH2 `.
```c
#define EDHOC_CONF_SH256 CC2538_SH2
```
Additionally other paramets can be definded, every definde parameter with their default values are settingon the `edhoc-config.h` files. For example the number of attempts and the timeout can be set by:
```c
#define EDHOC_CONF_ATTEMPTS 3

#define EDHOC_CONF_TIMEOUT 10000
```
## EDHOC dependencies
The EDHOC module implementation depends on the following libraries:

### ECDH Operation
The EDHOC module need to generate the ephemeral Diffie-Hellman pairs key and the EDHOC shared secrets. The `EDHOC_CONF_ECC` macro can be defined at the config file to choose between the following ECDH libraries:
#### Micro - ECC(UECC)
A small and fast ECDH and ECDSA SW implementation for 8-bit, 32-bit, and 64-bit processors. Implements five standard NIST curves, with `secp256r1` among them. The library is implemented in C and can be optionally optimised for either speed or code size at compilation time.
The specific external repository is added as a submodule in `os/net/security/uecc` folder.
- Author: K.Mackay
- Link:[MicroECC] (https:/*github.com/kmackay/micro-ecc/tree/601bd11062c551b108adbb43ba99f199b840777c) 
### cc2538-ecc-algo cc2538 ECC
Algorithms(CC2538)
This is a implementation of ECDH, ECDSA sign and ECDSA verify. It uses `ecc-driver`, the river for the cc2538 ECC mode of the PKC engine, to communicate with the PKA.It uses continuations to free the main CPU/thread while the PKA is calculating. It address the hardware acceleration of the ECDH cryptography operations.

### SH2(Secure Hash Algorithm)
The EDHOC module use SHA-256 (Secure Hash Algorithm) in HMAC-SHA256 (Hash-based message authentication) for key derivation functionalities in order to compute the transcript-hash(TH). The `EDHOC_CONF_SHA` macro can be defined at the config file to choose between the following SH256 libraries:
- SHA HW for CC2538 SH2 modules: Contiki-NG driver for the cc2538 SHA-256 mode of the security core.
- SHA SW library from Oriol Pinol [sha] (https:/*github.com/oriolpinol/contiki/tree/master/apps/ecc): The SHA-256 is included under `os/net/security/sha` folder. 

### CBOR(Concise Binary Object Representation)
The EDHOC module uses CBOR to encode the EDHOC exchanging messages. The `cbor` Contiki-NG implementation has been added from `group-oscore` in `lib/cbor` folder.
- Author:Martin Gunnarsson
- Link:[group-oscore] (https:/*github.com/Gunzter/contiki-ng/tree/group_oscore/os/net/app-layer/coap/oscore-support) 

A `cbor_put_bytes_identifier()` function has been added in order to cover the EDHOC protocol requirements.

### COSE module
The EDHOC module uses the COSE_Encrypt0 struct encryption from [RFC8152] for cryptography as well as the COSE_key format. The required COSE functionality has been implemented in a lightweight module under `os/net/security/cose `.

## EDHOC Client API
The edhoc - client - API.h file contains the EDHOC interface to be used by the EDHOC Initiator part as CoAP client.

- `edhoc_client_run()` : Runs the EDHOC Initiator part.This function must be called from the EDHOC Initiator program to start the EDHOC protocol as Initiator.Runs a new process that implements all the EDHOC protocol and exits when the EDHOC protocol finishes successfully or the EDHOC_CONF_ATTEMPTS are exceeded.
- `edhoc_client_callback()` : This function checks the events trigger from the EDHOC client process and returns '1' when the EDHOC protocol successfuly ends, -1 when the EDHOC protocol max retries are exceeded, and 0 when the EDHOC client process is steel running.
- `edhoc_client_close()` : This function must be called after the Security Context is exported to free the allocated memory.
- Optionally some functionalities for getting and setting the Aditional Application Data of the EDHOC messages are provided.

Once the EDHOC Client process is successfully finished, the security context can be exported by using the ehdoc - exporter.h API.For example:
- `edhoc_exporter_oscore.h()` : This function is used to derive an OSCORE Security Context[RFC8613] from the EDHOC shared secret.

### EDHOC Client Example
An EDHOC Client Example is provided at `examples/edhoc-tests/edhoc-test-client.c `.
For the specific example the EDHOC Server IP must be selected on the project-conf file, its own Node Key Identity and, the EDHOC part as Initiator:

- Define the Server IP address working as Responder
```c
#define EDHOC_CONF_SERVER_EP "coap://[fd01::201:1:1:1]" /* Server IP for works in Cooja simulator */
```
- Define the PARTY as Initiator
```c
#define EDHOC_CONF_PART PART_I
```

## EDHOC Server API
The edhoc - server - API.h file contains the EDHOC interface to be used by the EDHOC Responder as CoAP Server

- `edhoc_server_process()` : This function must be called from a CoAP response POST handler to run the EDHOC protocol Responder party process.
- `edhoc_server_callback()` :  This function checks the events trigger from the EDHOC server protocol looking for the `SERV_FINSHED` event.
- `edhoc_server_init()` : This function activates the EDHOC well-known CoAP Resource(that runs edhoc_server_resource() functionality) at the uri-path defined on the WELL_KNOWN macro (`./well-known.edhoc` by default).
- `edhoc_server_start()` : This function gets the DH-static authentication pair keys of the Server by using the edhoc-key-storage API, creates a new EDHOC context and generates the DH-ephemeral key for the specific session. A new EDHOC protocol session must be created for each new EDHOC client transaction.
- `edhoc_server_close()` : This function must be called after the Security Context is exported to free the allocated memory.
- `edhoc_server_restart()` : This function Rest the EDHOC context to initiate a new EDHOC protocol session with a new client.

- Optionally some functionalities for getting and setting the Aditional Application Data of the EDHOC messages are provided.

From every client that the EDHOC server side is successfully finished, the security context to be used with the specific client can be exported by using the ehdoc - exporter.h API.For example:
-`edhoc_exporter_oscore.h()` : This function is used to derive an OSCORE Security Context [RFC8613] from the EDHOC shared secret.

### EDHOC Server Example

An EDHOC Server Example is provided at `examples/edhoc-tests/edhoc-test-server.c ` together with the corresponding EDHOC plug test resource at
`examples/edhoc-tests/res-edhoc.c`. The specific example runs the EDHOC Responder protocol part on the CoAP server. Can runs on
constrained device or natively at a host toghter with the RPL border router role. Also can runs on constraind device as RPL node by definde the `EDHOC_RPL_NODE` macro to 1.

The Server Identity must be selected at:                                                                        
```c
#define AUTH_SUBJECT_NAME "Server_key_identity"
```

## EDHOC Tests
The EDHOC implementation has been tested running both `example\edhoc-test\edhoc-test-client.c ` and `example\edhoc-test\edhoc-test-server.c ` on Cooja and Zolertia RE-Mote platforms. In the latter case, it is mandatory to either set the 32Mhz Clock or else disable the watchdog when the[MicroECC] library is ussing.Instead when the CC2538 ECC module is used there is not need of increase the CPU clock frequency or disable the watchdog.

```c
#define SYS_CTRL_CONF_SYS_DIV SYS_CTRL_CLOCK_CTRL_SYS_DIV_32MHZ

#define WATCHDOG_CONF_ENABLE 0x00000000
```

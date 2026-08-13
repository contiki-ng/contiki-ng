# Ephemeral Diffie-Hellman Over COSE (EDHOC) [RFC9528]

The [RFC9528] IETF Internet - RFC specifies Ephemeral Diffie-Hellman Over COSE (EDHOC), a very compact, and lightweight authenticated Diffie-Hellman key exchange with ephemeral keys that provides mutual authentication, perfect forward secrecy, and identity protection. It uses COSE [RFC9052] for cryptography, CBOR [RFC8949] for encoding, and CoAP [RFC7252] for transport and the main use case is to establish an OSCORE security context. The EDHOC exchange and the key derivation follow known protocol constructions such as [SIGMA], NISTSP-800-56A and HKDF [RFC5869].

## EDHOC in Contiki-NG
The Contiki-NG EDHOC module implements asymmetric key authentication by using static Diffie-Hellman keys. The authentication is provided by a Message Authentication Code (MAC) computed from an ephemeral-static ECDH shared secret which enables significant reductions in message sizes. The implementation has passed the interoperability test successfully in the IETF-Hackathon 110.

## Standards Compliance

This implementation is **fully compliant with RFC 9528 (EDHOC)** and has been verified against RFC 9529 test vectors.

### Verified Standards Compliance
- **RFC 9528** - EDHOC protocol specification
- **RFC 9529** - EDHOC test vectors (all tests passing)
- **RFC 8949** - CBOR encoding/decoding with proper handling of both unsigned (0x00-0x17) and signed (0x20-0x37) integer encodings for byte identifiers
- **RFC 9052/9053** - COSE cryptography (Encrypt0 and Sign1 structures)
- **RFC 7959** - CoAP Block-Wise Transfer for message fragmentation
- **RFC 8613** - OSCORE security context derivation

### Recent Fixes (2025)
The implementation has been updated to correctly handle CBOR integer encoding for byte identifiers according to RFC 9528 Section 3.3.2:
- **Fixed**: Connection Identifiers (CID) in the range 0x20-0x37 are now properly read as CBOR negative integers (major type 1)
- **Fixed**: Compact-encoded Key Identifiers (KID) with the same encoding are now correctly handled
- **Impact**: Full compliance with CBOR integer encoding rules, enabling use of the complete valid range for single-byte identifiers

### Implementation Scope
The implementation supports a focused subset of EDHOC features optimized for resource-constrained IoT devices:
- **Cipher Suites**: 2, 3 (AES-CCM variants)
- **Methods**: 0, 1, 2, 3 (all authentication methods)
- **Credentials**: KID (Key Identifier) and CCS (CWT Claims Set)
- **Connection Identifiers**: Single-byte and multi-byte CIDs
- **Message Flow**: Forward message flow (initiator to responder)

### Known Limitations
The following features are intentionally not implemented to minimize code size and memory footprint:
- Message_4 (optional confirmation message)
- EAD (External Authorization Data) items
- X.509 certificate credentials
- Additional cipher suites (0-1, 4-6, 24-25)

These limitations are design choices for embedded systems and do not affect RFC 9528 compliance for the supported feature set.

EDHOC consists of three messages (MSG1, MSG2 and, MSG3), plus an EDHOC error message (MSG_ERR) where each of them is a CBOR sequence. The current implementation transports these messages as an exchange of Confirmable CoAP [RFC725] messages where the CoAP client is the EDHOC Initiator and the
CoAP server is the EDHOC Responder. The MSG1 and MSG3 are transferred in POST requests and MSG2 in a 2.04 (Changed) response to the Uri-Path: 
"/.well-known/edhoc". When MSGs size is bigger than 64B the Block-Wise transfer mechanism [RFC7959] for fragmentation is being used.

Notice that the authentication keys must be established at the EDHOC key storage before running the EDHOC protocol. For this reason, an edhoc-key-storage.h() API function is provided in order to set the COSE_key with the correct struct format.

At the configuration file, the credential type used for authentication must be selected. Two types have been implemented:
- `EDHOC_CRED_KID` : The EDHOC exchanging a unique identity of the public authentication key to be retrieved. Before running the EDHOC protocol each party need at least a DH-static public key and a set of identities which is allowed to communicate with.
- `EDHOC_CRED_INCLUDE` : The EDHOC exchanging messages which include directly the actual credential (DH-static public key) formatted as a CCS (CWT Claims Set). The EDHOC protocol can runs without prior knowledge of the other peer. Each peer provisionally accepts the credentials of the other party until posterior authentication and verification.

### EDHOC configuration
The following macro must be defined on the configuration file:
- Define the KID of the authentication key used on this node.
```c
#define EDHOC_AUTH_KID KID
```
If the parties have agreed on an identity beside the public key, the "subject name" can be defined instead.
```c
#define EDHOC_AUTH_SUBJECT_NAME "Node_Key_Identity"
```
- Define the role taking on the EDHOC protocol. `EDHOC_INITIATOR` for the Initiator and `EDHOC_RESPONDER` for the Responder.
```c
#define EDHOC_CONF_ROLE EDHOC_INITIATOR
```
- Define the Connection Identifier(`CID`)
```c
#define EDHOC_CONF_CID 0x20
```
- Define the EDHOC method to use
```c
#define EDHOC_CONF_METHOD EDHOC_METHOD3
```
ECDH operations are routed through the Contiki-NG ECC driver in `os/services/ecc`, which currently uses the bundled `micro-ecc` software implementation. EDHOC busy-waits the driver to completion, so no per-backend selection is needed in EDHOC itself.

Additional parameters can be defined; every defined parameter with its default value is set in `edhoc-config.h`. For example, the number of attempts and the timeout can be set by:
```c
#define EDHOC_CONF_ATTEMPTS 3

#define EDHOC_CONF_TIMEOUT 10000
```
## EDHOC dependencies
The EDHOC module implementation depends on the following libraries:

### ECDH Operation
The EDHOC module needs to generate ephemeral Diffie-Hellman key pairs and EDHOC shared secrets. These operations go through the Contiki-NG ECC driver (`lib/ecc.h`, `os/services/ecc`), which currently wraps the bundled `micro-ecc` software implementation (`os/net/security/micro-ecc`).

### CBOR(Concise Binary Object Representation)
The EDHOC module uses CBOR to encode the EDHOC exchanging messages.
- Author:Martin Gunnarsson
- Link:[group-oscore] (https://github.com/Gunzter/contiki-ng/tree/group_oscore/os/net/app-layer/coap/oscore-support)

### COSE module
The EDHOC module uses COSE_Encrypt0 and COSE_Sign1 from [RFC9052] for cryptography and signing as well as the COSE_key format. The required COSE functionality has been implemented in a lightweight module under `os/net/security/cose`.

## EDHOC Client API
The EDHOC - client - API.h file contains the EDHOC interface to be used by the EDHOC Initiator role as CoAP client.

- `edhoc_client_run()` : Runs the EDHOC Initiator role. This function must be called from the EDHOC Initiator program to start the EDHOC protocol as Initiator. Runs a new process that implements all the EDHOC protocol and exits when the EDHOC protocol finishes successfully or the EDHOC_CONF_ATTEMPTS are exceeded.
- `edhoc_client_callback()` : This function checks the events trigger from the EDHOC client process and returns '1' when the EDHOC protocol successfuly ends, -1 when the EDHOC protocol max retries are exceeded, and 0 when the EDHOC client process is steel running.
- `edhoc_client_close()` : This function must be called after the Security Context is exported to free the allocated memory.
- Optionally some functionalities for getting and setting the Aditional Application Data of the EDHOC messages are provided.

Once the EDHOC Client process is successfully finished, the security context can be exported by using the ehdoc - exporter.h API. For example:
- `edhoc_exporter_oscore.h()` : This function is used to derive an OSCORE Security Context[RFC8613] from the EDHOC shared secret.

### EDHOC Client Example
An EDHOC Client Example is provided at `examples/edhoc/edhoc-client/edhoc-client-example.c`.
For the specific example the EDHOC Server IP must be selected on the project-conf file, its own Node Key Identity and, the EDHOC role as Initiator:

- Define the Server IP address working as Responder
```c
#define EDHOC_CONF_SERVER_EP "coap://[fd00::201:1:1:1]" /* Server IP for works in Cooja simulator */
```
- Define the ROLEY as Initiator
```c
#define EDHOC_CONF_ROLE EDHOC_INITIATOR
```

## EDHOC Server API
The EDHOC - server - API.h file contains the EDHOC interface to be used by the EDHOC Responder as CoAP Server

- `edhoc_server_process()` : This function must be called from a CoAP response POST handler to run the EDHOC protocol Responder party process.
- `edhoc_server_callback()` :  This function checks the events triggered by the EDHOC server protocol, reporting handshake completion (`SERV_HANDSHAKE_COMPLETE`) and handshake reset (`SERV_HANDSHAKE_RESET`).
- `edhoc_server_init()` : This function activates the EDHOC well-known CoAP Resource (that runs edhoc_server_resource() functionality) at the Uri-Path defined on the EDHOC_COAP_URI_PATH macro (`.well-known/edhoc` by default).
- `edhoc_server_start()` : This function gets the DH-static authentication pair keys of the Server by using the edhoc-key-storage API, creates a new EDHOC context and generates the DH-ephemeral key for the specific session. A new EDHOC protocol session must be created for each new EDHOC client transaction.
- `edhoc_server_close()` : This function must be called after the Security Context is exported to free the allocated memory.
- `edhoc_server_reset_handshake()` : This function resets the EDHOC handshake state to prepare for a new client connection.

- Optionally some functionalities for getting and setting the Additional Application Data of the EDHOC messages are provided.

From every client that the EDHOC server side is successfully finished, the security context to be used with the specific client can be exported by using the EDHOC - exporter.h API. For example:
-`edhoc_exporter_oscore.h()` : This function is used to derive an OSCORE Security Context [RFC8613] from the EDHOC shared secret.

### EDHOC Server Example

An EDHOC Server Example is provided at `examples/edhoc/edhoc-server/edhoc-server-example.c` together with the corresponding EDHOC resource at
`os/net/security/edhoc/server/res-edhoc.c`. The specific example runs the EDHOC Responder protocol role on the CoAP server.

The Server Identity must be selected at:
```c
#define EDHOC_AUTH_SUBJECT_NAME "Server_key_identity"
```

## EDHOC Tests
The EDHOC implementation has been tested running both `examples/edhoc/edhoc-client/` and `examples/edhoc/edhoc-server/` on Cooja and Zolertia RE-Mote platforms. In the latter case, it is mandatory to either set the 32Mhz Clock or else disable the watchdog when the[MicroECC] library is using. Instead when the CC2538 ECC module is used there is not need of increase the CPU clock frequency or disable the watchdog.

```c
#define SYS_CTRL_CONF_SYS_DIV SYS_CTRL_CLOCK_CTRL_SYS_DIV_32MHZ

#define WATCHDOG_CONF_ENABLE 0x00000000
```

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define LPM_CONF_MAX_PM 1

#define EDHOC_CONF_TIMEOUT 100000

#define EDHOC_CONF_CID 0x27  /* RFC 9529 C_R (responder CID) = -8 in CBOR */

// Large size to avoid usage of block-wise
#define COAP_MAX_CHUNK_SIZE 300

/* Set authentication KID based on credential type */
#ifdef MAKE_EDHOC_CREDS_INTEROP_SIGN
#define EDHOC_AUTH_KID 0x09
#elif defined(MAKE_EDHOC_CREDS_RFC9529_STATIC_DH)
#define EDHOC_AUTH_KID 0x27
#else
#define EDHOC_AUTH_KID 0x32  /* RFC 9529 server KID */
#endif

/* Define the party role on the EDHOC protocol as responder and the correlation method */
#define EDHOC_CONF_ROLE EDHOC_RESPONDER /* Server */

/* Define the authentication */
#define EDHOC_CONF_AUTHENT_TYPE EDHOC_CRED_KID

/* To tell EDHOC server example to start as network root */
#ifndef IS_NETWORK_ROUTING_ROOT
#define IS_NETWORK_ROUTING_ROOT 1
#endif /* IS_NETWORK_ROUTING_ROOT */

/* Set the supported cipher suites */
#define EDHOC_CONF_SUPPORTED_SUITE_1 EDHOC_CIPHERSUITE_2

/* Enable Cryptographically Secure PRNG for EDHOC ephemeral keys */
#define CSPRNG_CONF_ENABLED 1

#define LOG_CONF_WITH_COMPACT_BYTES 0
#define LOG_CONF_LEVEL_EDHOC LOG_LEVEL_DBG

#endif /* PROJECT_CONF_H_ */

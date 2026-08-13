#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define LPM_CONF_MAX_PM 1

#define EDHOC_CONF_TIMEOUT 100000

#define EDHOC_CONF_CID 0x37  /* RFC 9529 C_I (initiator CID) = -24 in CBOR */

// Large size to avoid block-wise
#define COAP_MAX_CHUNK_SIZE 300

#define EDHOC_AUTH_KID 0x2b

/* Define the coap server to connect with */
#define EDHOC_CONF_SERVER_EP "coap://[fd00::202:2:2:2]" /* Server IP for Cooja simulator */

/* Define the party role on the EDHOC protocol as Initiator and the correlation method */
#define EDHOC_CONF_ROLE EDHOC_INITIATOR

/* Define the authentication */
#define EDHOC_CONF_AUTHENT_TYPE EDHOC_CRED_KID

/* Set the supported cipher suites */
#define EDHOC_CONF_SUPPORTED_SUITE_1 EDHOC_CIPHERSUITE_2
#define EDHOC_CONF_SUPPORTED_SUITE_2 EDHOC_CIPHERSUITE_6

/* Enable Cryptographically Secure PRNG for EDHOC ephemeral keys */
#define CSPRNG_CONF_ENABLED 1

#define LOG_CONF_WITH_COMPACT_BYTES 0
#define LOG_CONF_LEVEL_EDHOC LOG_LEVEL_DBG

#endif /* PROJECT_CONF_H_ */
/** @} */

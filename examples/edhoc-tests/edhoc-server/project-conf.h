
#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define LPM_CONF_MAX_PM 1

#define IEEE802154_CONF_PANID 0xaaaa


/* Mandatory EDHOC definitions on Server*/
/* Define one kind of the following kind of identifiaction for the authentication key*/
//#define AUTH_SUBJECT_NAME "Serv_A"
#define AUTH_KID 0x07

/* Definde a value for the Conection Identifier*/
#define EDHOC_CID 0x20

/*Define the party rol on the EDHOC protocol as responder and the correlation method*/
#define EDHOC_CONF_PART PART_R /*Server */
#define EDHOC_CONF_CORR EXTERNAL_CORR_U 

/*To run with the test vector DH ephimeral keys used on the edhoc-v02 interoperability sesion*/
//#define EDHOC_TEST TEST_VECTOR
#define EDHOC_CONF_VERSION EDHOC_02

/*Define the authentication method*/
//#define EDHOC_CONF_AUTHENT_TYPE PRKI_2
#define EDHOC_CONF_AUTHENT_TYPE PRK_ID


/*Define the libray for SHA operations*/
#define EDHOC_CONF_SH256 DECC_SH2
//#define EDHOC_CONF_SH256 CC2538_SH2

/*Define the libray for ECDH operations*/
//#define EDHOC_CONF_ECC CC2538_ECC
#define EDHOC_CONF_ECC UECC_ECC


/*#define EDHOC_CONF_SUIT  X25519_2
#define EDHOC_CONF_SUIT_1 P256_2
#define EDHOC_CONF_SUIT_2 P256*/


/*To run EDHOC server as RPL node*/
//#define EDHOC_CONF_RPL_NODE 1

/* Definde the node as Border Router*/
/*#ifndef WEBSERVER_CONF_CFS_CONNS
#define WEBSERVER_CONF_CFS_CONNS 2
#endif

#ifndef BORDER_ROUTER_CONF_WEBSERVER
#define BORDER_ROUTER_CONF_WEBSERVER 1
#endif

#if BORDER_ROUTER_CONF_WEBSERVER
#define UIP_CONF_TCP 1
#endif
*/
/*May be necesary to defind one of the following macros when the UECC_ECC library is used and 
the target is an embedded device*/ 
#define WATCHDOG_CONF_ENABLE 0x00000000
//#define SYS_CTRL_CONF_SYS_DIV SYS_CTRL_CLOCK_CTRL_SYS_DIV_32MHZ

#define LOG_CONF_LEVEL_EDHOC LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_COAP LOG_LEVEL_DBG
/*#define LOG_CONF_LEVEL_TCPIP LOG_LEVEL_DBG */
#define LOG_CONF_LEVEL_IPV6 LOG_LEVEL_DBG 

#endif /* PROJECT_CONF_H_ */

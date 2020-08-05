#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define LPM_CONF_MAX_PM 1

/* Mandatory EDHOC definitions on Client*/
#define AUTH_KEY_IDENTITY "Node_101"
/*coap definition */

#define EDHOC_CONF_SERVER_EP "coap://[fd01::201:1:1:1]" /* Server IP for Cooja simulator*/
#define EDHOC_CONF_PART PART_I
//#define EDHOC_CONF_AUTHENT_TYPE PRK_ID /*Every part have the authentication key of the other before run edhoc protcol */
#define EDHOC_CONF_AUTHENT_TYPE PRKI /*The authentication keys are include at the ID_CRED_X in the edhoc protocol exchange*/

/*To run EDHOC client as RPL node*/
#define EDHOC_CONF_RPL_NODE 1

/*Set the number of attempts to connect with the EDHOC server*/
#define EDHOC_CONF_ATTEMPTS 3

/*EDHOC extra option defined at their default values*/
#define EDHOC_CONF_TIMEOUT 10000
#define EDHOC_CONF_MAX_PAYLOAD 255
#define EDHOC_CONF_CORR NON_EXTERNAL_CORR
#define EDHOC_CONF_SH256 DECC_SH2
#define EDHOC_CONF_MAX_AD_SZ 255 /*For test big messasges*/
#define EDHOC_CONF_ECC UECC
#define EDHOC_CONF_ALGORITHM_ID COSE_Algorithm_AES_CCM_16_64_128
#define EDHOC_CONF_SUIT P256


#define LOG_CONF_LEVEL_EDHOC LOG_LEVEL_INFO
/*#define LOG_CONF_LEVEL_RPL LOG_LEVEL_DBG */
/*#define LOG_CONF_LEVEL_COAP LOG_LEVEL_DBG */
/*#define LOG_CONF_LEVEL_TCPIP LOG_LEVEL_DBG */
/*#define LOG_CONF_LEVEL_IPV6 LOG_LEVEL_DBG */


//#define WATCHDOG_CONF_ENABLE 0x00000000
#define SYS_CTRL_CONF_SYS_DIV SYS_CTRL_CLOCK_CTRL_SYS_DIV_32MHZ

#endif /* PROJECT_CONF_H_ */

/** @} */
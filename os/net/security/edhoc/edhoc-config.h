/*
 * Copyright (c) 2020, Industrial Systems Institute (ISI), Patras, Greece
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * \file
 *      EDHOC configuration file
 * \author
 *      Lidia Pocero <pocero@isi.gr>
 */

/**
 * \addtogroup edhoc
 * @{
 */

#ifndef _EDHOC_CONFIG_H_
#define _EDHOC_CONFIG_H_

#define EDHOC_02 1
#define EDHOC_O4 2

#ifdef EDHOC_CONF_VERSION
#define EDHOC_VERSION EDHOC_CONF_VERSION
#else
#define EDHOC_VERSION EDHOC_O4
#endif  


/*SHA256 types*/
#define DECC_SH2 0       /*Macro to declare the use of SH2 Software library from Oriol Pinol*/
#define DCC2538_SH2 1    /*Macro to declare the use of SH2 Harware of the CC2538 module */

/**
 * \brief Set the SH2 library
 */
#ifdef EDHOC_CONF_SH256
#define SH256 EDHOC_CONF_SH256
#else
#define SH256 DECC_SH2
#endif

/*Correlation types*/
#define NON_EXTERNAL_CORR 0
#define EXTERNAL_CORR_U 1
#define EXTERNAL_CORR_V 2
#define EXTERNAL_CORR_UV 3

/**
 * \brief Set the Correlation type
 */
#ifdef EDHOC_CONF_CORR
#define CORR EDHOC_CONF_CORR
#else
#define CORR EXTERNAL_CORR_UV
#endif

/* Part definition */
#define PART_R 0   /*The Responder of the edhoc protocol*/
#define PART_I 1   /*The Initiator of the edhoc protocol*/

/**
 * \brief Set the EDHOC Protocol part
 */
#ifdef EDHOC_CONF_PART
#define PART EDHOC_CONF_PART
#else
#define PART PART_I
#endif

/*COSE_key parameters */
#define OKP 1  /*no implemented yet */
#define EC2 2
#define SYMETRIC 3  /*no implemented yet */

/*EDHOC Authentication Method Types: Initiator (I) | Responder (R) */
#define METH0 0                  /* Signature Key  | Signature Key   //no implemented yet */
#define METH1 1                  /* Signature Key  | Static DH Key   //no implemented yet */
#define METH2 2                  /* Static DH Key  | Signature Key   //no implemented yet */
#define METH3 3                  /* Static DH Key  | Static DH Key */
#define METH4 4                  /* PSK            | PSK             //no implemented yet */

/**
 * \brief Set the Authentication method
 */
#ifdef EDHOC_CONF_METHOD
#define METHOD EDHOC_CONF_METHOD
#else
#define METHOD METH3
#endif

/* Credential  Types*/
#define PRKI 1
#define PRK_ID 2
#define PRKI_2 3

/**
 * \brief Set the authentication credential type
 */
#ifdef EDHOC_CONF_AUTHENT_TYPE
#define AUTHENT_TYPE EDHOC_CONF_AUTHENT_TYPE
#else
#define AUTHENT_TYPE PRK_ID
#endif

/*#define AUTHENTICATION_KEY_LEN 32 //For Signature key, not yet implemented*/

/*cipher suit */
#define X25519 0 /*AES-CCM-16-64-128, (HMAC 256/256) SHA-256, X25519, EdDSA, Ed25519, AES-CCM-16-64-128, SHA-256  //no implemented yet * / */
#define X25519_2 1  /*AES-CCM-16-128-128,(HMAC 256/256) SHA-256, X25519, EdDSA, Ed25519, AES-CCM-16-64-128, SHA-256 */
#define P256 2    /*AES-CCM-16-64-128, (HMAC 256/256) SHA-256, P-256, ES256, P-256, AES-CCM-16-64-128, SHA-256 */
#define P256_2 3    /*AES-CCM-16-64-128, (HMAC 256/256) SHA-256, P-256, ES256, P-256, AES-CCM-16-64-128, SHA-256 */

/**
 * \brief Set the EDHOC cipher suit
 */

#ifndef EDHOC_MAX_SUITS 
#define EDHOC_MAX_SUITS 5
#endif

#ifdef EDHOC_CONF_SUIT
#define SUIT EDHOC_CONF_SUIT
#else
#define SUIT P256
#endif

#ifdef EDHOC_CONF_SUIT_1
#define SUIT_1 EDHOC_CONF_SUIT_1
#else
#define SUIT_1 -1
#endif
#ifdef EDHOC_CONF_SUIT_2
#define SUIT_2 EDHOC_CONF_SUIT_2
#else
#define SUIT_2 -1
#endif
#ifdef EDHOC_CONF_SUIT_3
#define SUIT_3 EDHOC_CONF_SUIT_3
#else
#define SUIT_3 -1
#endif
#ifdef EDHOC_CONF_SUIT_4
#define SUIT_4 EDHOC_CONF_SUIT_4
#else
#define SUIT_4 -1
#endif

/*Set COSE_Key parameter*/
//#if (SUIT == P256)
#define KEY_CRV 1
#define KEY_TYPE EC2 /*EC2 key */
//#endif
//#if (SUIT == X25519)
//#define KEY_CRV 1
//#define KEY_TYPE EC2 /*EC2 key */

//#define KEY_CRV 4
//#define KEY_TYPE OKP /*EC2 key */
//#endif

/**
 * \brief COSE algorithm selection
 */
#ifdef EDHOC_CONF_ALGORITHM_ID
#define ALGORITHM_ID EDHOC_CONF_ALGORITHM_ID
#define COSE_CONF_ALGORITHM_ID EDHOC_CONF_ALGORITHM_ID
#else
#define ALGORITHM_ID COSE_Algorithm_AES_CCM_16_64_128

#define COSE_CONF_ALGORITHM_ID COSE_Algorithm_AES_CCM_16_64_128
#endif

/* Selected Algorithm Parameters*/
#if ALGORITHM_ID == COSE_Algorithm_AES_CCM_16_64_128
#define ECC_KEY_BYTE_LENGHT 32
#define HAS_LENGHT 32
#define KEY_DATA_LENGHT COSE_algorithm_AES_CCM_16_64_128_KEY_LEN
#define IV_LENGHT COSE_algorithm_AES_CCM_16_64_128_IV_LEN
#endif

/**
 * \brief Set the Edhoc part as RPL node. By default deselected
 */
#ifdef EDHOC_CONF_RPL_NODE
#define RPL_NODE EDHOC_CONF_RPL_NODE
#else
#define RPL_NODE 0
#endif

/**
 * \brief Set the number of attempts to connect with the EDHOC server successfully
 */
#ifndef EDHOC_CONF_ATTEMPTS
#define EDHOC_CONF_ATTEMPTS 3
#endif

/**
 * \brief The max lenght of the EDHOC msg, as CoAP payload
 */
#ifdef EDHOC_CONF_MAX_PAYLOAD
#define MAX_DATA_LEN EDHOC_CONF_MAX_PAYLOAD
#else
#define MAX_DATA_LEN 254
#endif

/**
 * \brief The max lenght of the Aplication Data
 */
#ifdef EDHOC_CONF_MAX_AD_SZ
#define MAX_AD_SZ EDHOC_CONF_MAX_AD_SZ
#else
#define MAX_AD_SZ 16
#endif

/**
 * \brief EDHOC resource urti-path
 */
#define WELL_KNOWN ".well-known/edhoc"

#endif /* _EDHOC_CONFIG_H_ */

/** @} */

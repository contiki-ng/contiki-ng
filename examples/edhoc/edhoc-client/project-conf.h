/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden AB
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
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* EDHOC Client Configuration */
#define EDHOC_CONF_ROLE EDHOC_INITIATOR
#define EDHOC_CONF_METHOD EDHOC_METHOD0
#define EDHOC_CONF_TIMEOUT 100000
#define EDHOC_CONF_CID 0x37

/* CoAP configuration */
#define COAP_MAX_OPEN_TRANSACTIONS   4
#define COAP_MAX_OBSERVERS          8
#define COAP_MAX_CHUNK_SIZE         300

/* Network configuration */
#define UIP_CONF_MAX_ROUTES         30
#define RPL_CONF_MAX_PARENTS         8

/* EDHOC Authentication Configuration */
#define EDHOC_AUTH_KID 0x2b
#define EDHOC_CONF_AUTHENT_TYPE EDHOC_CRED_KID

/* EDHOC Cipher Suite Configuration */
#define EDHOC_CONF_SUPPORTED_SUITE_1 EDHOC_CIPHERSUITE_2
#define EDHOC_CONF_SUPPORTED_SUITE_2 EDHOC_CIPHERSUITE_6

/* EDHOC Test Configuration */
#define EDHOC_CONF_TEST EDHOC_TEST_VECTOR_TRACE_DH

/* Logging levels */
#define LOG_CONF_LEVEL_EDHOC        LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_COAP         LOG_LEVEL_WARN
#define LOG_CONF_WITH_COMPACT_BYTES 0

/* Server endpoint configuration */
#define EDHOC_CONF_SERVER_EP "coap://[fd00::202:2:2:2]"

/* Low Power Mode Configuration.
   Cap the power mode at PM1 so that the whole SRAM is usable on cc2538-based
   boards (in PM2 only the full-retention SRAM bank is available, which is too
   small for this example). Matches the EDHOC server configuration. */
#define LPM_CONF_MAX_PM 1

#endif /* PROJECT_CONF_H_ */

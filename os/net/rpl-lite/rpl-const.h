/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \addtogroup rpl-lite
 * @{
 *
 * \file
 *	Constants for RPL
 * \author
 *	Joakim Eriksson <joakime@sics.se> & Nicolas Tsiftes <nvt@sics.se>,
 *  Simon Duquennoy <simon.duquennoy@inria.fr>
 *
 */

#ifndef RPL_CONST_H
#define RPL_CONST_H

/********** Constants **********/

/* Special value indicating infinite lifetime. */
#define RPL_INFINITE_LIFETIME                 0xFF
#define RPL_ROUTE_INFINITE_LIFETIME           0xFFFFFFFF
#define RPL_INFINITE_RANK                     0xFFFF

/*---------------------------------------------------------------------------*/
/* IANA Routing Metric/Constraint Type as defined in RFC6551 */
#define RPL_DAG_MC_NONE			            0 /* Local identifier for empty MC */
#define RPL_DAG_MC_NSA                  1 /* Node State and Attributes */
#define RPL_DAG_MC_ENERGY               2 /* Node Energy */
#define RPL_DAG_MC_HOPCOUNT             3 /* Hop Count */
#define RPL_DAG_MC_THROUGHPUT           4 /* Throughput */
#define RPL_DAG_MC_LATENCY              5 /* Latency */
#define RPL_DAG_MC_LQL                  6 /* Link Quality Level */
#define RPL_DAG_MC_ETX                  7 /* Expected Transmission Count */
#define RPL_DAG_MC_LC                   8 /* Link Color */

/* IANA Routing Metric/Constraint Common Header Flag field as defined in RFC6551 (bit indexes) */
#define RPL_DAG_MC_FLAG_P               5
#define RPL_DAG_MC_FLAG_C               6
#define RPL_DAG_MC_FLAG_O               7
#define RPL_DAG_MC_FLAG_R               8

/* IANA Routing Metric/Constraint Common Header A Field as defined in RFC6551 */
#define RPL_DAG_MC_AGGR_ADDITIVE        0
#define RPL_DAG_MC_AGGR_MAXIMUM         1
#define RPL_DAG_MC_AGGR_MINIMUM         2
#define RPL_DAG_MC_AGGR_MULTIPLICATIVE  3

/* The bit index within the flags field of the rpl_metric_object_energy structure. */
#define RPL_DAG_MC_ENERGY_INCLUDED	     3
#define RPL_DAG_MC_ENERGY_TYPE		       1
#define RPL_DAG_MC_ENERGY_ESTIMATION	   0

/* IANA Node Type Field as defined in RFC6551 */
#define RPL_DAG_MC_ENERGY_TYPE_MAINS		    0
#define RPL_DAG_MC_ENERGY_TYPE_BATTERY		  1
#define RPL_DAG_MC_ENERGY_TYPE_SCAVENGING   2

/* IANA Objective Code Point as defined in RFC6550 */
#define RPL_OCP_OF0     0
#define RPL_OCP_MRHOF   1

/*---------------------------------------------------------------------------*/
/* RPL message types */
#define RPL_CODE_DIS                   0x00   /* DAG Information Solicitation */
#define RPL_CODE_DIO                   0x01   /* DAG Information Option */
#define RPL_CODE_DAO                   0x02   /* Destination Advertisement Option */
#define RPL_CODE_DAO_ACK               0x03   /* DAO acknowledgment */
#define RPL_CODE_SEC_DIS               0x80   /* Secure DIS */
#define RPL_CODE_SEC_DIO               0x81   /* Secure DIO */
#define RPL_CODE_SEC_DAO               0x82   /* Secure DAO */
#define RPL_CODE_SEC_DAO_ACK           0x83   /* Secure DAO-ACK */

/* RPL control message options. */
#define RPL_OPTION_PAD1                  0
#define RPL_OPTION_PADN                  1
#define RPL_OPTION_DAG_METRIC_CONTAINER  2
#define RPL_OPTION_ROUTE_INFO            3
#define RPL_OPTION_DAG_CONF              4
#define RPL_OPTION_TARGET                5
#define RPL_OPTION_TRANSIT               6
#define RPL_OPTION_SOLICITED_INFO        7
#define RPL_OPTION_PREFIX_INFO           8
#define RPL_OPTION_TARGET_DESC           9

#define RPL_DAO_K_FLAG                   0x80 /* DAO-ACK requested */
#define RPL_DAO_D_FLAG                   0x40 /* DODAG ID present */

#define RPL_DAO_ACK_UNCONDITIONAL_ACCEPT 0
#define RPL_DAO_ACK_ACCEPT               1   /* 1 - 127 is OK but not good */
#define RPL_DAO_ACK_UNABLE_TO_ACCEPT     128 /* >127 is fail */
#define RPL_DAO_ACK_UNABLE_TO_ADD_ROUTE_AT_ROOT 255 /* root can not accept */
#define RPL_DAO_ACK_TIMEOUT              -1

/*---------------------------------------------------------------------------*/
/* RPL IPv6 extension header option. */
#define RPL_HDR_OPT_LEN			               4
#define RPL_HOP_BY_HOP_LEN		             (RPL_HDR_OPT_LEN + 2 + 2)
#define RPL_RH_LEN                         4
#define RPL_SRH_LEN                        4
#define RPL_RH_TYPE_SRH                    3
#define RPL_HDR_OPT_DOWN		               0x80
#define RPL_HDR_OPT_DOWN_SHIFT  	         7
#define RPL_HDR_OPT_RANK_ERR		           0x40
#define RPL_HDR_OPT_RANK_ERR_SHIFT   	     6
#define RPL_HDR_OPT_FWD_ERR		             0x20
#define RPL_HDR_OPT_FWD_ERR_SHIFT   	     5

/*---------------------------------------------------------------------------*/
#define RPL_INSTANCE_LOCAL_FLAG         0x80
#define RPL_INSTANCE_D_FLAG             0x40

/* Values that tell where a route came from. */
#define RPL_ROUTE_FROM_INTERNAL         0
#define RPL_ROUTE_FROM_UNICAST_DAO      1
#define RPL_ROUTE_FROM_MULTICAST_DAO    2
#define RPL_ROUTE_FROM_DIO              3

/* DAG Mode of Operation */
#define RPL_MOP_NO_DOWNWARD_ROUTES      0
#define RPL_MOP_NON_STORING             1
#define RPL_MOP_STORING_NO_MULTICAST    2
#define RPL_MOP_STORING_MULTICAST       3

 /** @} */

#endif /* RPL_CONST_H */

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
 *	Public configuration and API declarations for ContikiRPL.
 * \author
 *	Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>,
 *  Simon DUquennoy <simon.duquennoy@inria.fr>
 *
 */

#ifndef RPL_CONF_H
#define RPL_CONF_H

#include "contiki.h"

/******************************************************************************/
/*********************** Enabling/disabling features **************************/
/******************************************************************************/

/* RPL Mode of operation */
#ifdef  RPL_CONF_MOP
#define RPL_MOP_DEFAULT                 RPL_CONF_MOP
#else /* RPL_CONF_MOP */
#define RPL_MOP_DEFAULT                 RPL_MOP_NON_STORING
#endif /* RPL_CONF_MOP */

/*
 * We only support non-storing mode
 */
#define RPL_WITH_STORING 0

/*
 * Embed support for non-storing mode
 */
#ifdef RPL_CONF_WITH_NON_STORING
#define RPL_WITH_NON_STORING RPL_CONF_WITH_NON_STORING
#else /* RPL_CONF_WITH_NON_STORING */
/* By default: embed support for non-storing if and only if the configured MOP is non-storing */
#define RPL_WITH_NON_STORING (RPL_MOP_DEFAULT == RPL_MOP_NON_STORING)
#endif /* RPL_CONF_WITH_NON_STORING */

/*
 * The objective function (OF) used by a RPL root is configurable through
 * the RPL_CONF_OF_OCP parameter. This is defined as the objective code
 * point (OCP) of the OF, RPL_OCP_OF0 or RPL_OCP_MRHOF. This flag is of
 * no relevance to non-root nodes, which run the OF advertised in the
 * instance they join.
 * Make sure the selected of is inRPL_SUPPORTED_OFS.
 */
#ifdef RPL_CONF_OF_OCP
#define RPL_OF_OCP RPL_CONF_OF_OCP
#else /* RPL_CONF_OF_OCP */
#define RPL_OF_OCP RPL_OCP_MRHOF
#endif /* RPL_CONF_OF_OCP */

/*
 * The set of objective functions supported at runtime. Nodes are only
 * able to join instances that advertise an OF in this set. To include
 * both OF0 and MRHOF, use {&rpl_of0, &rpl_mrhof}.
 */
#ifdef RPL_CONF_SUPPORTED_OFS
#define RPL_SUPPORTED_OFS RPL_CONF_SUPPORTED_OFS
#else /* RPL_CONF_SUPPORTED_OFS */
#define RPL_SUPPORTED_OFS {&rpl_mrhof}
#endif /* RPL_CONF_SUPPORTED_OFS */

/*
 * Enable/disable RPL Metric Containers (MC). The actual MC in use
 * for a given DODAG is decided at runtime, when joining. Note that
 * OF0 (RFC6552) operates without MC, and so does MRHOF (RFC6719) when
 * used with ETX as a metric (the rank is the metric). We disable MC
 * by default, but note it must be enabled to support joining a DODAG
 * that requires MC (e.g., MRHOF with a metric other than ETX).
 */
#ifdef RPL_CONF_WITH_MC
#define RPL_WITH_MC RPL_CONF_WITH_MC
#else /* RPL_CONF_WITH_MC */
#define RPL_WITH_MC 0
#endif /* RPL_CONF_WITH_MC */

/* The MC advertised in DIOs and propagating from the root */
#ifdef RPL_CONF_DAG_MC
#define RPL_DAG_MC RPL_CONF_DAG_MC
#else
#define RPL_DAG_MC RPL_DAG_MC_NONE
#endif /* RPL_CONF_DAG_MC */

/*
 * RPL DAO-ACK support. When enabled, DAO-ACK will be sent and requested.
 * This will also enable retransmission of DAO when no ack is received.
 * */
#ifdef RPL_CONF_WITH_DAO_ACK
#define RPL_WITH_DAO_ACK RPL_CONF_WITH_DAO_ACK
#else
#define RPL_WITH_DAO_ACK 1
#endif /* RPL_CONF_WITH_DAO_ACK */

/*
 * Setting the RPL_TRICKLE_REFRESH_DAO_ROUTES will make the RPL root
 * increase the DTSN (Destination Advertisement Trigger Sequence Number)
 * from the DIO trickle timer. If set to 4, DTSN will be increased every 4th
 * iteration. This is to get all children to re-register their DAO route.
 * This is needed when DAO-ACK is not enabled to
 * add reliability to route maintenance.
 * */
#ifdef RPL_CONF_TRICKLE_REFRESH_DAO_ROUTES
#define RPL_TRICKLE_REFRESH_DAO_ROUTES RPL_CONF_TRICKLE_REFRESH_DAO_ROUTES
#else
#if RPL_WITH_DAO_ACK
#define RPL_TRICKLE_REFRESH_DAO_ROUTES 0
#else
#define RPL_TRICKLE_REFRESH_DAO_ROUTES 4
#endif
#endif /* RPL_CONF_TRICKLE_REFRESH_DAO_ROUTES */

/*
 * RPL probing. When enabled, probes will be sent periodically to keep
 * neighbor link estimates up to date. Further configurable
 * via RPL_CONF_PROBING_* flags
 */
#ifdef RPL_CONF_WITH_PROBING
#define RPL_WITH_PROBING RPL_CONF_WITH_PROBING
#else
#define RPL_WITH_PROBING 1
#endif

/*
 * Function used to select the next neighbor to be probed.
 */
#ifdef RPL_CONF_PROBING_SELECT_FUNC
#define RPL_PROBING_SELECT_FUNC RPL_CONF_PROBING_SELECT_FUNC
#else
#define RPL_PROBING_SELECT_FUNC get_probing_target
#endif

/*
 * Function used to send RPL probes.
 * To probe with DIO, use:
 * #define RPL_CONF_PROBING_SEND_FUNC(addr) rpl_icmp6_dio_output((addr))
 * To probe with DIS, use:
 * #define RPL_CONF_PROBING_SEND_FUNC(addr) rpl_icmp6_dis_output((addr))
 * Any other custom probing function is also acceptable.
 */
#ifdef RPL_CONF_PROBING_SEND_FUNC
#define RPL_PROBING_SEND_FUNC RPL_CONF_PROBING_SEND_FUNC
#else
#define RPL_PROBING_SEND_FUNC(addr) rpl_icmp6_dio_output((addr))
#endif

/*
 * This value decides if this node must stay as a leaf or not
 * as allowed by draft-ietf-roll-rpl-19#section-8.5
 */
#ifdef RPL_CONF_DEFAULT_LEAF_ONLY
#define RPL_DEFAULT_LEAF_ONLY RPL_CONF_DEFAULT_LEAF_ONLY
#else
#define RPL_DEFAULT_LEAF_ONLY 0
#endif

/*
 * Function used to validate dio before using it to init dag
 */
#ifdef RPL_CONF_VALIDATE_DIO_FUNC
#define RPL_VALIDATE_DIO_FUNC RPL_CONF_VALIDATE_DIO_FUNC
#endif

/******************************************************************************/
/********************************** Timing ************************************/
/******************************************************************************/

/*
 * The DIO interval (n) represents 2^n ms.
 *
 * According to the specification, the default value is 3 which
 * means 8 milliseconds. That is far too low when using duty cycling
 * with wake-up intervals that are typically hundreds of milliseconds.
 * ContikiRPL thus sets the default to 2^12 ms = 4.096 s.
 */
#ifdef RPL_CONF_DIO_INTERVAL_MIN
#define RPL_DIO_INTERVAL_MIN        RPL_CONF_DIO_INTERVAL_MIN
#else
#define RPL_DIO_INTERVAL_MIN        12
#endif

/*
 * Maximum amount of timer doublings.
 *
 * The maximum interval will by default be 2^(12+8) ms = 1048.576 s.
 * RFC 6550 suggests a default value of 20, which of course would be
 * unsuitable when we start with a minimum interval of 2^12.
 */
#ifdef RPL_CONF_DIO_INTERVAL_DOUBLINGS
#define RPL_DIO_INTERVAL_DOUBLINGS  RPL_CONF_DIO_INTERVAL_DOUBLINGS
#else
#define RPL_DIO_INTERVAL_DOUBLINGS  8
#endif

/*
 * DIO redundancy. To learn more about this, see RFC 6206.
 *
 * RFC 6550 suggests a default value of 10. We disable this mechanism by
 * default, using a value of 0. This is to enable reliable DTSN increment
 * propagation, and periodic rank dissemination without the need to increment
 * version. Note that Trickle was originally designed for CTP, aiming to
 * reduce broadcast traffic, which was particularly expensive done over an
 * LPL MAC. In MAC layers such as non-beacon enabled or TSCH, broadcast is no
 * more costly than unicast. Further, in this RPL implementation, DIOs are
 * responsible for only a portion of the control traffic, compared to Link
 * probing which is done at a period of minutes (RPL_PROBING_INTERVAL)
 * for reliable parent selection.
 */
#ifdef RPL_CONF_DIO_REDUNDANCY
#define RPL_DIO_REDUNDANCY          RPL_CONF_DIO_REDUNDANCY
#else
#define RPL_DIO_REDUNDANCY          0
#endif

/*
 * Default route lifetime unit. This is the granularity of time
 * used in RPL lifetime values, in seconds.
 */
#ifndef RPL_CONF_DEFAULT_LIFETIME_UNIT
#define RPL_DEFAULT_LIFETIME_UNIT       60
#else
#define RPL_DEFAULT_LIFETIME_UNIT       RPL_CONF_DEFAULT_LIFETIME_UNIT
#endif

/*
 * Default route lifetime as a multiple of the lifetime unit.
 */
#ifndef RPL_CONF_DEFAULT_LIFETIME
#define RPL_DEFAULT_LIFETIME            30
#else
#define RPL_DEFAULT_LIFETIME            RPL_CONF_DEFAULT_LIFETIME
#endif

/* Maximum lifetime of a DAG as a multiple of the lifetime unit. */
#ifdef RPL_CONF_DAG_LIFETIME
#define RPL_DAG_LIFETIME                RPL_CONF_DAG_LIFETIME
#else
#define RPL_DAG_LIFETIME                (8 * 60) /* 8 hours */
#endif /* RPL_CONF_DAG_LIFETIME */

/*
 * RPL probing interval.
 */
#ifdef RPL_CONF_PROBING_INTERVAL
#define RPL_PROBING_INTERVAL RPL_CONF_PROBING_INTERVAL
#else
#define RPL_PROBING_INTERVAL (90 * CLOCK_SECOND)
#endif

/*
 * Function used to calculate next RPL probing interval
 */
#ifdef RPL_CONF_PROBING_DELAY_FUNC
#define RPL_PROBING_DELAY_FUNC RPL_CONF_PROBING_DELAY_FUNC
#else
#define RPL_PROBING_DELAY_FUNC get_probing_delay
#endif

/* Poisoining duration, before leaving the DAG  */
#ifdef RPL_CONF_DELAY_BEFORE_LEAVING
#define RPL_DELAY_BEFORE_LEAVING        RPL_CONF_DELAY_BEFORE_LEAVING
#else
#define RPL_DELAY_BEFORE_LEAVING        (5 * 60 * CLOCK_SECOND)
#endif

/* Interval of DIS transmission  */
#ifdef RPL_CONF_DIS_INTERVAL
#define RPL_DIS_INTERVAL                RPL_CONF_DIS_INTERVAL
#else
#define RPL_DIS_INTERVAL                (30 * CLOCK_SECOND)
#endif

/* DAO transmissions are always delayed by RPL_DAO_DELAY +/- RPL_DAO_DELAY/2 */
#ifdef RPL_CONF_DAO_DELAY
#define RPL_DAO_DELAY                 RPL_CONF_DAO_DELAY
#else /* RPL_CONF_DAO_DELAY */
#define RPL_DAO_DELAY                 (CLOCK_SECOND * 4)
#endif /* RPL_CONF_DAO_DELAY */

#ifdef RPL_CONF_DAO_MAX_RETRANSMISSIONS
#define RPL_DAO_MAX_RETRANSMISSIONS RPL_CONF_DAO_MAX_RETRANSMISSIONS
#else
#define RPL_DAO_MAX_RETRANSMISSIONS       5
#endif /* RPL_CONF_DAO_MAX_RETRANSMISSIONS */

#ifdef RPL_CONF_DAO_RETRANSMISSION_TIMEOUT
#define RPL_DAO_RETRANSMISSION_TIMEOUT  RPL_CONF_DAO_RETRANSMISSION_TIMEOUT
#else
#define RPL_DAO_RETRANSMISSION_TIMEOUT    (5 * CLOCK_SECOND)
#endif /* RPL_CONF_DAO_RETRANSMISSION_TIMEOUT */

/******************************************************************************/
/************************** More parameterization *****************************/
/******************************************************************************/

#ifndef RPL_CONF_MIN_HOPRANKINC
/* RFC6550 defines the default MIN_HOPRANKINC as 256.
 * However, we use MRHOF as a default Objective Function (RFC6719),
 * which recommends setting MIN_HOPRANKINC with care, in particular
 * when used with ETX as a metric. ETX is computed as a fixed point
 * real with a divisor of 128 (RFC6719, RFC6551). We choose to also
 * use 128 for RPL_MIN_HOPRANKINC, resulting in a rank equal to the
 * ETX path cost. Larger values may also be desirable, as discussed
 * in section 6.1 of RFC6719. */
#if RPL_OF_OCP == RPL_OCP_MRHOF
#define RPL_MIN_HOPRANKINC          128
#else /* RPL_OF_OCP == RPL_OCP_MRHOF */
#define RPL_MIN_HOPRANKINC          256
#endif /* RPL_OF_OCP == RPL_OCP_MRHOF */
#else /* RPL_CONF_MIN_HOPRANKINC */
#define RPL_MIN_HOPRANKINC          RPL_CONF_MIN_HOPRANKINC
#endif /* RPL_CONF_MIN_HOPRANKINC */

#ifndef RPL_CONF_MAX_RANKINC
#define RPL_MAX_RANKINC             (8 * RPL_MIN_HOPRANKINC)
#else /* RPL_CONF_MAX_RANKINC */
#define RPL_MAX_RANKINC             RPL_CONF_MAX_RANKINC
#endif /* RPL_CONF_MAX_RANKINC */

#ifndef RPL_CONF_SIGNIFICANT_CHANGE_THRESHOLD
#define RPL_SIGNIFICANT_CHANGE_THRESHOLD             (4 * RPL_MIN_HOPRANKINC)
#else /* RPL_CONF_SIGNIFICANT_CHANGE_THRESHOLD */
#define RPL_SIGNIFICANT_CHANGE_THRESHOLD             RPL_CONF_SIGNIFICANT_CHANGE_THRESHOLD
#endif /* RPL_CONF_SIGNIFICANT_CHANGE_THRESHOLD */

/* This value decides which DAG instance we should participate in by default. */
#ifdef RPL_CONF_DEFAULT_INSTANCE
#define RPL_DEFAULT_INSTANCE RPL_CONF_DEFAULT_INSTANCE
#else
#define RPL_DEFAULT_INSTANCE	          0 /* Default of 0 for compression */
#endif /* RPL_CONF_DEFAULT_INSTANCE */

/* Set to have the root advertise a grounded DAG */
#ifndef RPL_CONF_GROUNDED
#define RPL_GROUNDED                    0
#else
#define RPL_GROUNDED                    RPL_CONF_GROUNDED
#endif /* !RPL_CONF_GROUNDED */

/*
 * DAG preference field
 */
#ifdef RPL_CONF_PREFERENCE
#define RPL_PREFERENCE              RPL_CONF_PREFERENCE
#else
#define RPL_PREFERENCE              0
#endif

/* RPL callbacks when TSCH is enabled */
#if MAC_CONF_WITH_TSCH

#ifndef RPL_CALLBACK_PARENT_SWITCH
#define RPL_CALLBACK_PARENT_SWITCH tsch_rpl_callback_parent_switch
#endif /* RPL_CALLBACK_PARENT_SWITCH */

#ifndef RPL_CALLBACK_NEW_DIO_INTERVAL
#define RPL_CALLBACK_NEW_DIO_INTERVAL tsch_rpl_callback_new_dio_interval
#endif /* RPL_CALLBACK_NEW_DIO_INTERVAL */

#endif /* MAC_CONF_WITH_TSCH */

/* Set to 1 to drop packets when a forwarding loop is detected
 * on a packet that already had an error signaled, as per RFC6550 - 11.2.2.2.
 * Disabled by default for more reliability: even in the event of a loop,
 * packets get a chance to eventually find their way to the destination. */
#ifdef RPL_CONF_LOOP_ERROR_DROP
#define RPL_LOOP_ERROR_DROP RPL_CONF_LOOP_ERROR_DROP
#else /* RPL_CONF_LOOP_ERROR_DROP */
#define RPL_LOOP_ERROR_DROP 0
#endif /* RPL_CONF_LOOP_ERROR_DROP */

/** @} */

#endif /* RPL_CONF_H */

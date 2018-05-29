/*
 * Copyright (c) 2018, RISE SICS.
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
 * \addtogroup simple-energest
 * @{
 */

 /**
  * \file
  *         A process that periodically prints out the time spent in
  *         radio tx, radio rx, total time and duty cycle.
  *
  * \author Simon Duquennoy <simon.duquennoy@ri.se>
  */

#ifndef SIMPLE_ENERGEST_H_
#define SIMPLE_ENERGEST_H_

/** \brief The period at which Energest statistics will be logged */
#ifdef SIMPLE_ENERGEST_CONF_PERIOD
#define SIMPLE_ENERGEST_PERIOD SIMPLE_ENERGEST_CONF_PERIOD
#else /* SIMPLE_ENERGEST_CONF_PERIOD */
#define SIMPLE_ENERGEST_PERIOD (CLOCK_SECOND * 60)
#endif /* SIMPLE_ENERGEST_CONF_PERIOD */

/**
 * Initialize the deployment module
 */
void simple_energest_init(void);

#endif /* SIMPLE_ENERGEST_H_ */
/** @} */

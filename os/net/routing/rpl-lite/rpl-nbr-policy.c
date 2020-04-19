/*
 * Copyright (c) 2020, Yanzi Networks AB.
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

 /**
  * \addtogroup uip
  * @{
  */

#include "net/routing/rpl-lite/rpl.h"
#include "net/nbr-table.h"
#include "net/link-stats.h"
#include "net/ipv6/uip-ds6-route.h"
#include "sys/log.h"

#define LOG_MODULE "RPL-nbrpol"
#define LOG_LEVEL LOG_LEVEL_NONE

/*---------------------------------------------------------------------------*/
static rpl_rank_t
get_rank(const linkaddr_t *lladdr)
{
  rpl_parent_t *p = rpl_neighbor_get_from_lladdr((uip_lladdr_t *)lladdr);
  if(p == NULL) {
    return RPL_INFINITE_RANK;
  } else {
    return curr_instance.of->rank_via_nbr(p);
  }
}
/*---------------------------------------------------------------------------*/
const linkaddr_t *
rpl_nbr_gc_get_worst(const linkaddr_t *lladdr1, const linkaddr_t *lladdr2)
{
  return get_rank(lladdr2) > get_rank(lladdr1) ? lladdr2 : lladdr1;
}
/*---------------------------------------------------------------------------*/
/** @}*/

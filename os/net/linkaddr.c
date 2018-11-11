/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 * \file
 *         Functions for manipulating link-layer addresses
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

/**
 * \addtogroup linkaddr
 * @{
 */

#include "net/linkaddr.h"
#include <string.h>
#include <stdio.h>

linkaddr_t linkaddr_node_addr;
#if LINKADDR_SIZE == 2
const linkaddr_t linkaddr_null = { { 0, 0 } };
#else /*LINKADDR_SIZE == 2*/
#if LINKADDR_SIZE == 8
const linkaddr_t linkaddr_null = { { 0, 0, 0, 0, 0, 0, 0, 0 } };
#endif /*LINKADDR_SIZE == 8*/
#if LINKADDR_SIZE == 6
const linkaddr_t linkaddr_null = { { 0, 0, 0, 0, 0, 0 } };
#endif /*LINKADDR_SIZE == 6*/
#endif /*LINKADDR_SIZE == 2*/


/*---------------------------------------------------------------------------*/
void
linkaddr_copy(linkaddr_t *dest, const linkaddr_t *src)
{
	memcpy(dest, src, LINKADDR_SIZE);
}
/*---------------------------------------------------------------------------*/
int
linkaddr_cmp(const linkaddr_t *addr1, const linkaddr_t *addr2)
{
	return (memcmp(addr1, addr2, LINKADDR_SIZE) == 0);
}
/*---------------------------------------------------------------------------*/
void
linkaddr_set_node_addr(linkaddr_t *t)
{
  linkaddr_copy(&linkaddr_node_addr, t);
}
/*---------------------------------------------------------------------------*/
int
linkaddr_from_string(linkaddr_t *addr, const char *addr_str)
{
  unsigned int values[LINKADDR_SIZE];
  int i;
  int byte_count;

  /*
   * End formats with %*c to get rid of any unexpected characters beyond the
   * end of the expected format.
   */
  switch(LINKADDR_SIZE) {
  case 2:
    byte_count = sscanf(addr_str, "%2x:%2x%*c", &values[0], &values[1]);
    break;
  case 6:
    byte_count = sscanf(addr_str, "%2x:%2x:%2x:%2x:%2x:%2x%*c",
                        &values[0], &values[1], &values[2],
                        &values[3], &values[4], &values[5]);
    break;
  case 8:
    byte_count = sscanf(addr_str, "%2x:%2x:%2x:%2x:%2x:%2x:%2x:%2x%*c",
                        &values[0], &values[1], &values[2],
                        &values[3], &values[4], &values[5],
                        &values[6], &values[7]);
    break;
  default:
    return -1;
  }

  /*
   * Return an error unless we converted exactly LINKADDR_SIZE bytes.
   * Otherwise convert unsigned int to uint8_t before copying to the
   * destination address.
   */
  if(byte_count == LINKADDR_SIZE) {
    for(i = 0; i < LINKADDR_SIZE; ++i)
      addr->u8[i] = (uint8_t)values[i];

    return byte_count;
  }

  return -1;
}
/*---------------------------------------------------------------------------*/
/** @} */

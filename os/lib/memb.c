/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
 * \addtogroup memb
 * @{
 */

 /**
 * \file
 * Memory block allocation routines.
 * \author Adam Dunkels <adam@sics.se>
 */
#include <string.h>

#include "contiki.h"
#include "lib/memb.h"

/*---------------------------------------------------------------------------*/
void
memb_init(struct memb *m)
{
  memset(m->used, 0, m->num);
  memset(m->mem, 0, m->size * m->num);
}
/*---------------------------------------------------------------------------*/
void *
memb_alloc(struct memb *m)
{
  int i;

  for(i = 0; i < m->num; ++i) {
    if(m->used[i] == false) {
      /* If this block was unused, we set the used flag on
	 and return a pointer to the memory block. */
      m->used[i] = true;
      return (void *)((char *)m->mem + (i * m->size));
    }
  }

  /* No free block was found, so we return NULL to indicate failure to
     allocate block. */
  return NULL;
}
/*---------------------------------------------------------------------------*/
int
memb_free(struct memb *m, void *ptr)
{
  int i;
  char *ptr2;

  /* Walk through the list of blocks and try to find the block to
     which the pointer "ptr" points to. */
  ptr2 = (char *)m->mem;
  for(i = 0; i < m->num; ++i) {
    if(ptr2 == (char *)ptr) {
      /* We've found the block to which "ptr" points, so we check the allocation
         status to detect the double-free error and free the block. */
      if (m->used[i] == false)
        return -1;
      m->used[i] = false;
      return 0;
    }
    ptr2 += m->size;
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int
memb_inmemb(struct memb *m, void *ptr)
{
  return (char *)ptr >= (char *)m->mem &&
    (char *)ptr < (char *)m->mem + (m->num * m->size);
}
/*---------------------------------------------------------------------------*/
size_t
memb_numfree(struct memb *m)
{
  int i;
  size_t num_free = 0;

  for(i = 0; i < m->num; ++i) {
    if(m->used[i] == false) {
      ++num_free;
    }
  }

  return num_free;
}
/** @} */

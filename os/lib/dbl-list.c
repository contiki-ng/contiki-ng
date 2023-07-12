/*
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
 * Copyright (c) 2017, James Pope
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup doubly-linked-list
 * @{
 *
 * \file
 *   Implementation of doubly-linked lists
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/dbl-list.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
struct dll {
  struct dll *next;
  struct dll *previous;
};
/*---------------------------------------------------------------------------*/
void
dbl_list_init(dbl_list_t dll)
{
  *dll = NULL;
}
/*---------------------------------------------------------------------------*/
void *
dbl_list_head(const_dbl_list_t dll)
{
  return *dll;
}
/*---------------------------------------------------------------------------*/
void *
dbl_list_tail(const_dbl_list_t dll)
{
  struct dll *this;

  if(*dll == NULL) {
    return NULL;
  }

  for(this = *dll; this->next != NULL; this = this->next);

  return this;
}
/*---------------------------------------------------------------------------*/
void
dbl_list_remove(dbl_list_t dll, const void *element)
{
  struct dll *this, *previous, *next;

  if(*dll == NULL || element == NULL) {
    return;
  }

  for(this = *dll; this != NULL; this = this->next) {
    if(this == element) {
      previous = this->previous;
      next = this->next;

      if(previous) {
        previous->next = this->next;
      }

      if(next) {
        next->previous = this->previous;
      }

      if(*dll == this) {
        *dll = next;
      }

      return;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
dbl_list_add_head(dbl_list_t dll, void *element)
{
  struct dll *head;

  if(element == NULL) {
    return;
  }

  /* Don't add twice */
  dbl_list_remove(dll, element);

  head = dbl_list_head(dll);

  ((struct dll *)element)->previous = NULL;
  ((struct dll *)element)->next = head;

  if(head) {
    /* If the list was not empty, update ->previous on the old head */
    head->previous = element;
  }

  *dll = element;
}
/*---------------------------------------------------------------------------*/
void
dbl_list_add_tail(dbl_list_t dll, void *element)
{
  struct dll *tail;

  if(element == NULL) {
    return;
  }

  /* Don't add twice */
  dbl_list_remove(dll, element);

  tail = dbl_list_tail(dll);

  if(tail == NULL) {
    /* The list was empty */
    *dll = element;
  } else {
    tail->next = element;
  }

  ((struct dll *)element)->previous = tail;
  ((struct dll *)element)->next = NULL;
}
/*---------------------------------------------------------------------------*/
void
dbl_list_add_after(dbl_list_t dll, void *existing, void *element)
{
  if(element == NULL || existing == NULL) {
    return;
  }

  /* Don't add twice */
  dbl_list_remove(dll, element);

  ((struct dll *)element)->next = ((struct dll *)existing)->next;
  ((struct dll *)element)->previous = existing;

  if(((struct dll *)existing)->next) {
    ((struct dll *)existing)->next->previous = element;
  }
  ((struct dll *)existing)->next = element;
}
/*---------------------------------------------------------------------------*/
void
dbl_list_add_before(dbl_list_t dll, void *existing, void *element)
{
  if(element == NULL || existing == NULL) {
    return;
  }

  /* Don't add twice */
  dbl_list_remove(dll, element);

  ((struct dll *)element)->next = existing;
  ((struct dll *)element)->previous = ((struct dll *)existing)->previous;

  if(((struct dll *)existing)->previous) {
    ((struct dll *)existing)->previous->next = element;
  }
  ((struct dll *)existing)->previous = element;

  /* If we added before the list's head, we must update the head */
  if(*dll == existing) {
    *dll = element;
  }
}
/*---------------------------------------------------------------------------*/
unsigned long
dbl_list_length(const_dbl_list_t dll)
{
  unsigned long len = 0;
  struct dll *this;

  if(*dll == NULL) {
    return 0;
  }

  for(this = *dll; this != NULL; this = this->next) {
    len++;
  }

  return len;
}
/*---------------------------------------------------------------------------*/
bool
dbl_list_is_empty(const_dbl_list_t dll)
{
  return *dll == NULL ? true : false;
}
/*---------------------------------------------------------------------------*/
/** @} */

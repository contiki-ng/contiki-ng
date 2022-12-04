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
 * \addtogroup doubly-linked-circular-list
 * @{
 *
 * \file
 *   Implementation of circular, doubly-linked lists
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/dbl-circ-list.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
struct dblcl {
  struct dblcl *next;
  struct dblcl *previous;
};
/*---------------------------------------------------------------------------*/
void
dbl_circ_list_init(dbl_circ_list_t dblcl)
{
  *dblcl = NULL;
}
/*---------------------------------------------------------------------------*/
void *
dbl_circ_list_head(const_dbl_circ_list_t dblcl)
{
  return *dblcl;
}
/*---------------------------------------------------------------------------*/
void *
dbl_circ_list_tail(const_dbl_circ_list_t dblcl)
{
  struct dblcl *this;

  if(*dblcl == NULL) {
    return NULL;
  }

  for(this = *dblcl; this->next != *dblcl; this = this->next);

  return this;
}
/*---------------------------------------------------------------------------*/
void
dbl_circ_list_remove(dbl_circ_list_t dblcl, const void *element)
{
  struct dblcl *this;

  if(*dblcl == NULL || element == NULL) {
    return;
  }

  this = *dblcl;

  do {
    if(this == element) {
      this->previous->next = this->next;
      this->next->previous = this->previous;

      /* We need to update the head of the list if we removed the head */
      if(*dblcl == element) {
        *dblcl = this->next == this ? NULL : this->next;
      }

      this->next = NULL;
      this->previous = NULL;

      return;
    }

    this = this->next;
  } while(this != *dblcl);
}
/*---------------------------------------------------------------------------*/
void
dbl_circ_list_add_head(dbl_circ_list_t dblcl, void *element)
{
  struct dblcl *head;

  if(element == NULL) {
    return;
  }

  /* Don't add twice */
  dbl_circ_list_remove(dblcl, element);

  head = dbl_circ_list_head(dblcl);

  if(head == NULL) {
    /* If the list was empty */
    ((struct dblcl *)element)->next = element;
    ((struct dblcl *)element)->previous = element;
  } else {
    /* If the list was not empty */
    ((struct dblcl *)element)->next = head;
    ((struct dblcl *)element)->previous = head->previous;
    head->previous->next = element;
    head->previous = element;
  }

  *dblcl = element;
}
/*---------------------------------------------------------------------------*/
void
dbl_circ_list_add_tail(dbl_circ_list_t dblcl, void *element)
{
  struct dblcl *tail;

  if(element == NULL) {
    return;
  }

  /* Don't add twice */
  dbl_circ_list_remove(dblcl, element);

  tail = dbl_circ_list_tail(dblcl);

  if(tail == NULL) {
    /* If the list was empty */
    ((struct dblcl *)element)->next = element;
    ((struct dblcl *)element)->previous = element;
    *dblcl = element;
  } else {
    /* If the list was not empty */
    ((struct dblcl *)element)->next = *dblcl;
    ((struct dblcl *)element)->previous = tail;
    tail->next->previous = element;
    tail->next = element;
  }
}
/*---------------------------------------------------------------------------*/
void
dbl_circ_list_add_after(dbl_circ_list_t dblcl, void *existing, void *element)
{
  if(element == NULL || existing == NULL) {
    return;
  }

  /* Don't add twice */
  dbl_circ_list_remove(dblcl, element);

  ((struct dblcl *)element)->next = ((struct dblcl *)existing)->next;
  ((struct dblcl *)element)->previous = existing;
  ((struct dblcl *)existing)->next->previous = element;
  ((struct dblcl *)existing)->next = element;
}
/*---------------------------------------------------------------------------*/
void
dbl_circ_list_add_before(dbl_circ_list_t dblcl, void *existing, void *element)
{
  if(element == NULL || existing == NULL) {
    return;
  }

  /* Don't add twice */
  dbl_circ_list_remove(dblcl, element);

  ((struct dblcl *)element)->next = existing;
  ((struct dblcl *)element)->previous = ((struct dblcl *)existing)->previous;
  ((struct dblcl *)existing)->previous->next = element;
  ((struct dblcl *)existing)->previous = element;

  /* If we added before the list's head, we must update the head */
  if(*dblcl == existing) {
    *dblcl = element;
  }
}
/*---------------------------------------------------------------------------*/
unsigned long
dbl_circ_list_length(const_dbl_circ_list_t dblcl)
{
  unsigned long len = 1;
  struct dblcl *this;

  if(*dblcl == NULL) {
    return 0;
  }

  for(this = *dblcl; this->next != *dblcl; this = this->next) {
    len++;
  }

  return len;
}
/*---------------------------------------------------------------------------*/
bool
dbl_circ_list_is_empty(const_dbl_circ_list_t dblcl)
{
  return *dblcl == NULL ? true : false;
}
/*---------------------------------------------------------------------------*/
/** @} */

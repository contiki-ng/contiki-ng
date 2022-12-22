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
 * \addtogroup circular-singly-linked-list
 * @{
 *
 * \file
 *   Implementation of circular singly linked lists
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/circular-list.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
struct cl {
  struct cl *next;
};
/*---------------------------------------------------------------------------*/
void
circular_list_init(circular_list_t cl)
{
  *cl = NULL;
}
/*---------------------------------------------------------------------------*/
void *
circular_list_head(const_circular_list_t cl)
{
  return *cl;
}
/*---------------------------------------------------------------------------*/
void *
circular_list_tail(const_circular_list_t cl)
{
  struct cl *this;

  if(*cl == NULL) {
    return NULL;
  }

  for(this = *cl; this->next != *cl; this = this->next);

  return this;
}
/*---------------------------------------------------------------------------*/
void
circular_list_remove(circular_list_t cl, const void *element)
{
  struct cl *this, *previous;

  if(*cl == NULL) {
    return;
  }

  /*
   * We start traversing from the second element.
   * The head will be visited last. We always update the list's head after
   * removal, just in case we have just removed the head.
   */
  previous = *cl;
  this = previous->next;

  do {
    if(this == element) {
      previous->next = this->next;
      *cl = this->next == this ? NULL : previous;
      return;
    }
    previous = this;
    this = this->next;
  } while(this != ((struct cl *)*cl)->next);
}
/*---------------------------------------------------------------------------*/
void
circular_list_add(circular_list_t cl, void *element)
{
  struct cl *head;

  if(element == NULL) {
    return;
  }

  /* Don't add twice */
  circular_list_remove(cl, element);

  head = *cl;

  if(head == NULL) {
    /* If the list was empty, we update the new element to point to itself */
    ((struct cl *)element)->next = element;
  } else {
    /* If the list exists, we add the new element between the current head and
     * the previously second element. */
    ((struct cl *)element)->next = head->next;
    head->next = element;
  }

  /* In all cases, the new element becomes the list's new head */
  *cl = element;
}
/*---------------------------------------------------------------------------*/
unsigned long
circular_list_length(const_circular_list_t cl)
{
  unsigned long len = 1;
  struct cl *this;

  if(circular_list_is_empty(cl)) {
    return 0;
  }

  for(this = *cl; this->next != *cl; this = this->next) {
    len++;
  }

  return len;
}
/*---------------------------------------------------------------------------*/
bool
circular_list_is_empty(const_circular_list_t cl)
{
  return *cl == NULL ? true : false;
}
/*---------------------------------------------------------------------------*/
/** @} */

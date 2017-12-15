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
#include "contiki.h"
#include "lib/stack.h"
#include "lib/queue.h"
#include "lib/circular-list.h"
#include "lib/dbl-list.h"
#include "lib/dbl-circ-list.h"
#include "lib/random.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
PROCESS(data_structure_process, "Data structure process");
AUTOSTART_PROCESSES(&data_structure_process);
/*---------------------------------------------------------------------------*/
STACK(demo_stack);
QUEUE(demo_queue);
CIRCULAR_LIST(demo_cll);
DBL_LIST(demo_dbl);
DBL_CIRC_LIST(demo_dblcl);
/*---------------------------------------------------------------------------*/
typedef struct demo_struct_s {
  struct demo_struct_s *next;
  struct demo_struct_s *previous;
  unsigned short value;
} demo_struct_t;
/*---------------------------------------------------------------------------*/
#define DATA_STRUCTURE_DEMO_ELEMENT_COUNT 4
static demo_struct_t elements[DATA_STRUCTURE_DEMO_ELEMENT_COUNT];
/*---------------------------------------------------------------------------*/
static void
dbl_circ_list_print(dbl_circ_list_t dblcl)
{
  demo_struct_t *this = *dblcl;

  if(*dblcl == NULL) {
    printf("Length=0\n");
    return;
  }

  do {
    printf("<--(0x%04x)--0x%04x--(0x%04x)-->", this->previous->value,
           this->value, this->next->value);
    this = this->next;
  } while(this != *dblcl);

  printf(" (Length=%lu)\n", dbl_circ_list_length(dblcl));
}
/*---------------------------------------------------------------------------*/
static void
demonstrate_dbl_circ_list(void)
{
  int i;
  demo_struct_t *this;

  dbl_circ_list_init(demo_dblcl);
  printf("============================\n");
  printf("Circular, doubly-linked list\n");

  for(i = 0; i < DATA_STRUCTURE_DEMO_ELEMENT_COUNT; i++) {
    elements[i].next = NULL;
    elements[i].previous = NULL;
  }

  /* Add elements */
  dbl_circ_list_add_tail(demo_dblcl, &elements[0]);
  printf("Add tail  : 0x%04x | ", elements[0].value);
  dbl_circ_list_print(demo_dblcl);

  dbl_circ_list_add_after(demo_dblcl, &elements[0], &elements[1]);
  printf("Add after : 0x%04x | ", elements[1].value);
  dbl_circ_list_print(demo_dblcl);

  dbl_circ_list_add_head(demo_dblcl, &elements[2]);
  printf("Add head  : 0x%04x | ", elements[2].value);
  dbl_circ_list_print(demo_dblcl);

  dbl_circ_list_add_before(demo_dblcl, &elements[2], &elements[3]);
  printf("Add before: 0x%04x | ", elements[3].value);
  dbl_circ_list_print(demo_dblcl);

  /* Remove head */
  this = dbl_circ_list_head(demo_dblcl);
  printf("Rm head: (0x%04x)  | ", this->value);
  dbl_circ_list_remove(demo_dblcl, this);
  dbl_circ_list_print(demo_dblcl);

  /* Remove currently second element */
  this = ((demo_struct_t *)dbl_circ_list_head(demo_dblcl))->next;
  printf("Rm 2nd : (0x%04x)  | ", this->value);
  dbl_circ_list_remove(demo_dblcl, this);
  dbl_circ_list_print(demo_dblcl);

  /* Remove tail */
  this = dbl_circ_list_tail(demo_dblcl);
  printf("Rm tail: (0x%04x)  | ", this->value);
  dbl_circ_list_remove(demo_dblcl, this);
  dbl_circ_list_print(demo_dblcl);

  /* Remove last remaining element */
  this = dbl_circ_list_tail(demo_dblcl);
  printf("Rm last: (0x%04x)  | ", this->value);
  dbl_circ_list_remove(demo_dblcl, this);
  dbl_circ_list_print(demo_dblcl);

  printf("Circular, doubly-linked list is%s empty\n",
         dbl_circ_list_is_empty(demo_dblcl) ? "" : " not");
}
/*---------------------------------------------------------------------------*/
static void
dbl_list_print(dbl_list_t dll)
{
  demo_struct_t *this;

  for(this = *dll; this != NULL; this = this->next) {
    printf("<--(");
    if(this->previous == NULL) {
      printf(" null ");
    } else {
      printf("0x%04x", this->previous->value);
    }

    printf(")--0x%04x--(", this->value);

    if(this->next == NULL) {
      printf(" null ");
    } else {
      printf("0x%04x", this->next->value);
    }
    printf(")-->");
  }

  printf(" (Length=%lu)\n", dbl_list_length(dll));
}
/*---------------------------------------------------------------------------*/
static void
demonstrate_dbl_list(void)
{
  int i;
  demo_struct_t *this;

  dbl_list_init(demo_dbl);
  printf("==================\n");
  printf("Doubly-linked list\n");

  for(i = 0; i < DATA_STRUCTURE_DEMO_ELEMENT_COUNT; i++) {
    elements[i].next = NULL;
    elements[i].previous = NULL;
  }

  /* Add elements */
  dbl_list_add_tail(demo_dbl, &elements[0]);
  printf("Add tail  : 0x%04x | ", elements[0].value);
  dbl_list_print(demo_dbl);

  dbl_list_add_after(demo_dbl, &elements[0], &elements[1]);
  printf("Add after : 0x%04x | ", elements[1].value);
  dbl_list_print(demo_dbl);

  dbl_list_add_head(demo_dbl, &elements[2]);
  printf("Add head  : 0x%04x | ", elements[2].value);
  dbl_list_print(demo_dbl);

  dbl_list_add_before(demo_dbl, &elements[2], &elements[3]);
  printf("Add before: 0x%04x | ", elements[3].value);
  dbl_list_print(demo_dbl);

  /* Remove head */
  this = dbl_list_head(demo_dbl);
  printf("Rm head: (0x%04x)  | ", this->value);
  dbl_list_remove(demo_dbl, this);
  dbl_list_print(demo_dbl);

  /* Remove currently second element */
  this = ((demo_struct_t *)dbl_list_head(demo_dbl))->next;
  printf("Rm 2nd : (0x%04x)  | ", this->value);
  dbl_list_remove(demo_dbl, this);
  dbl_list_print(demo_dbl);

  /* Remove tail */
  this = dbl_list_tail(demo_dbl);
  printf("Rm tail: (0x%04x)  | ", this->value);
  dbl_list_remove(demo_dbl, this);
  dbl_list_print(demo_dbl);

  /* Remove last remaining element */
  this = dbl_list_tail(demo_dbl);
  printf("Rm last: (0x%04x)  | ", this->value);
  dbl_list_remove(demo_dbl, this);
  dbl_list_print(demo_dbl);

  printf("Doubly-linked list is%s empty\n",
         dbl_list_is_empty(demo_dbl) ? "" : " not");
}
/*---------------------------------------------------------------------------*/
static void
circular_list_print(circular_list_t cl)
{
  demo_struct_t *this = *cl;

  if(*cl == NULL) {
    printf("Length=0\n");
    return;
  }

  do {
    printf("0x%04x-->", this->value);
    this = this->next;
  } while(this != *cl);

  printf("0x%04x (Length=%lu)\n", this->value, circular_list_length(cl));
}
/*---------------------------------------------------------------------------*/
static void
demonstrate_circular_list(void)
{
  int i;

  circular_list_init(demo_cll);
  printf("============================\n");
  printf("Circular, singly-linked list\n");

  /* Add elements */
  for(i = 0; i < DATA_STRUCTURE_DEMO_ELEMENT_COUNT; i++) {
    elements[i].next = NULL;
    circular_list_add(demo_cll, &elements[i]);

    printf("Add: 0x%04x | ", elements[i].value);
    circular_list_print(demo_cll);
  }

  /* Remove head */
  circular_list_remove(demo_cll, circular_list_head(demo_cll));
  printf("Remove head | ");
  circular_list_print(demo_cll);

  /* Remove currently second element */
  circular_list_remove(demo_cll,
                       ((demo_struct_t *)circular_list_head(demo_cll))->next);
  printf("Remove 2nd  | ");
  circular_list_print(demo_cll);

  /* Remove tail */
  circular_list_remove(demo_cll, circular_list_tail(demo_cll));
  printf("Remove tail | ");
  circular_list_print(demo_cll);

  /* Remove last remaining element */
  circular_list_remove(demo_cll, circular_list_tail(demo_cll));
  printf("Remove last | ");
  circular_list_print(demo_cll);

  printf("Circular list is%s empty\n",
         circular_list_is_empty(demo_cll) ? "" : " not");
}
/*---------------------------------------------------------------------------*/
static void
demonstrate_stack(void)
{
  int i;
  demo_struct_t *this;

  printf("=====\n");
  printf("Stack\n");

  stack_init(demo_stack);

  /* Add elements */
  for(i = 0; i < DATA_STRUCTURE_DEMO_ELEMENT_COUNT; i++) {
    elements[i].next = NULL;
    stack_push(demo_stack, &elements[i]);
    printf("Push: 0x%04x\n", elements[i].value);
  }

  printf("Peek: 0x%04x\n",
         ((demo_struct_t *)stack_peek(demo_stack))->value);

  for(i = 0; i <= DATA_STRUCTURE_DEMO_ELEMENT_COUNT; i++) {
    this = stack_pop(demo_stack);
    printf("Pop: ");
    if(this == NULL) {
      printf("(stack underflow)\n");
    } else {
      printf("0x%04x\n", this->value);
    }
  }
  printf("Stack is%s empty\n",
         stack_is_empty(demo_stack) ? "" : " not");
}
/*---------------------------------------------------------------------------*/
static void
demonstrate_queue(void)
{
  int i;
  demo_struct_t *this;

  printf("=====\n");
  printf("Queue\n");

  queue_init(demo_queue);

  /* Add elements */
  for(i = 0; i < DATA_STRUCTURE_DEMO_ELEMENT_COUNT; i++) {
    elements[i].next = NULL;
    queue_enqueue(demo_queue, &elements[i]);
    printf("Enqueue: 0x%04x\n", elements[i].value);
  }

  printf("Peek: 0x%04x\n",
         ((demo_struct_t *)queue_peek(demo_queue))->value);

  for(i = 0; i <= DATA_STRUCTURE_DEMO_ELEMENT_COUNT; i++) {
    this = queue_dequeue(demo_queue);
    printf("Dequeue: ");
    if(this == NULL) {
      printf("(queue underflow)\n");
    } else {
      printf("0x%04lx\n", (unsigned long)this->value);
    }
  }

  printf("Queue is%s empty\n",
         queue_is_empty(demo_queue) ? "" : " not");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(data_structure_process, ev, data)
{
  int i;

  PROCESS_BEGIN();

  /* Generate some elements */
  printf("Elements: [");

  for(i = 0; i < DATA_STRUCTURE_DEMO_ELEMENT_COUNT; i++) {
    elements[i].next = NULL;
    elements[i].value = random_rand();
    printf(" 0x%04x", elements[i].value);
  }
  printf(" ]\n");

  demonstrate_stack();
  demonstrate_queue();
  demonstrate_circular_list();
  demonstrate_dbl_list();
  demonstrate_dbl_circ_list();

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

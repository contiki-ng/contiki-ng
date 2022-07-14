/*
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
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
#include "lib/list.h"
#include "lib/stack.h"
#include "lib/queue.h"
#include "lib/circular-list.h"
#include "lib/dbl-list.h"
#include "lib/dbl-circ-list.h"
#include "lib/random.h"
#include "services/unit-test/unit-test.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
PROCESS(data_structure_test_process, "Data structure process");
AUTOSTART_PROCESSES(&data_structure_test_process);
/*---------------------------------------------------------------------------*/
typedef struct demo_struct_s {
  struct demo_struct_s *next;
  struct demo_struct_s *previous;
} demo_struct_t;
/*---------------------------------------------------------------------------*/
#define ELEMENT_COUNT 10
static demo_struct_t elements[ELEMENT_COUNT];
/*---------------------------------------------------------------------------*/
void
print_test_report(const unit_test_t *utp)
{
  printf("=check-me= ");
  if(utp->passed == false) {
    printf("FAILED   - %s: exit at L%u\n", utp->descr, utp->exit_line);
  } else {
    printf("SUCCEEDED - %s\n", utp->descr);
  }
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_list, "Singly-linked list");
UNIT_TEST(test_list)
{
  demo_struct_t *head, *tail;

  LIST(lst);

  UNIT_TEST_BEGIN();

  memset(elements, 0, sizeof(elements));
  list_init(lst);

  /* Starts from empty */
  UNIT_TEST_ASSERT(list_head(lst) == NULL);
  UNIT_TEST_ASSERT(list_length(lst) == 0);

  /*
   * Add an item. Should be head and tail
   * 0 --> NULL
   */
  list_add(lst, &elements[0]);
  UNIT_TEST_ASSERT(list_head(lst) == &elements[0]);
  UNIT_TEST_ASSERT(list_tail(lst) == &elements[0]);
  UNIT_TEST_ASSERT(elements[0].next == NULL);
  UNIT_TEST_ASSERT(list_length(lst) == 1);

  /*
   * Add an item. Should become the new tail
   * 0 --> 1 --> NULL
   */
  list_add(lst, &elements[1]);
  head = list_head(lst);
  tail = list_tail(lst);
  UNIT_TEST_ASSERT(list_head(lst) == &elements[0]);
  UNIT_TEST_ASSERT(list_tail(lst) == &elements[1]);
  UNIT_TEST_ASSERT(head->next == tail);
  UNIT_TEST_ASSERT(tail->next == NULL);
  UNIT_TEST_ASSERT(list_length(lst) == 2);

  /*
   * Add after existing head
   * 0 --> 2 --> 1 --> NULL
   */
  head = list_head(lst);
  list_insert(lst, head, &elements[2]);
  head = list_head(lst);
  tail = list_tail(lst);
  UNIT_TEST_ASSERT(list_head(lst) == &elements[0]);
  UNIT_TEST_ASSERT(list_tail(lst) == &elements[1]);
  UNIT_TEST_ASSERT(head->next == &elements[2]);
  UNIT_TEST_ASSERT(elements[2].next == tail);
  UNIT_TEST_ASSERT(tail->next == NULL);
  UNIT_TEST_ASSERT(list_length(lst) == 3);

  /*
   * Add after existing head
   * 0 --> 2 --> 1 --> 3 --> NULL
   */
  tail = list_tail(lst);
  list_insert(lst, tail, &elements[3]);
  tail = list_tail(lst);
  UNIT_TEST_ASSERT(list_head(lst) == &elements[0]);
  UNIT_TEST_ASSERT(list_tail(lst) == &elements[3]);
  UNIT_TEST_ASSERT(tail->next == NULL);
  UNIT_TEST_ASSERT(list_length(lst) == 4);

  /*
   * Re-add item 2 using list_add
   * 0 --> 1 --> 3 --> 2 --> NULL
   */
  list_add(lst, &elements[2]);
  tail = list_tail(lst);
  UNIT_TEST_ASSERT(list_head(lst) == &elements[0]);
  UNIT_TEST_ASSERT(list_tail(lst) == &elements[2]);
  UNIT_TEST_ASSERT(tail->next == NULL);
  UNIT_TEST_ASSERT(list_length(lst) == 4);

  /*
   * Re-add item 3 using list_insert
   * 0 --> 1 --> 2 --> 3 --> NULL
   */
  tail = list_tail(lst);
  list_insert(lst, tail, &elements[3]);
  tail = list_tail(lst);
  UNIT_TEST_ASSERT(list_head(lst) == &elements[0]);
  UNIT_TEST_ASSERT(list_tail(lst) == &elements[3]);
  UNIT_TEST_ASSERT(elements[2].next == tail);
  UNIT_TEST_ASSERT(tail->next == NULL);
  UNIT_TEST_ASSERT(list_length(lst) == 4);

  /*
   * Check that list contains elements 0, 1, 2, 3
   * and not others
   * 0 --> 1 --> 2 --> 3 --> NULL
   */
  UNIT_TEST_ASSERT(list_contains(lst, &elements[0]));
  UNIT_TEST_ASSERT(list_contains(lst, &elements[1]));
  UNIT_TEST_ASSERT(list_contains(lst, &elements[2]));
  UNIT_TEST_ASSERT(list_contains(lst, &elements[3]));
  int i;
  for(i=4; i<ELEMENT_COUNT; i++) {
    UNIT_TEST_ASSERT(!list_contains(lst, &elements[i]));
  }
  /*
   * Remove the tail
   * 0 --> 1 --> 2 --> NULL
   */
  tail = list_tail(lst);
  UNIT_TEST_ASSERT(list_chop(lst) == tail);
  head = list_head(lst);
  tail = list_tail(lst);
  UNIT_TEST_ASSERT(tail == &elements[2]);
  UNIT_TEST_ASSERT(tail->next == NULL);
  UNIT_TEST_ASSERT(list_length(lst) == 3);

  /*
   * Remove an item in the middle
   * 0 --> 2 --> NULL
   */
  head = list_head(lst);
  list_remove(lst, head->next);
  head = list_head(lst);
  tail = list_tail(lst);
  UNIT_TEST_ASSERT(head->next == tail);
  UNIT_TEST_ASSERT(list_head(lst) == &elements[0]);
  UNIT_TEST_ASSERT(list_tail(lst) == &elements[2]);
  UNIT_TEST_ASSERT(tail->next == NULL);
  UNIT_TEST_ASSERT(list_length(lst) == 2);

  /*
   * Remove the head
   * 2 --> NULL
   */
  list_remove(lst, list_head(lst));
  tail = list_tail(lst);
  UNIT_TEST_ASSERT(list_head(lst) == &elements[2]);
  UNIT_TEST_ASSERT(list_tail(lst) == &elements[2]);
  UNIT_TEST_ASSERT(tail->next == NULL);
  UNIT_TEST_ASSERT(list_length(lst) == 1);

  /* Ends empty */
  list_remove(lst, list_tail(lst));
  UNIT_TEST_ASSERT(list_head(lst) == NULL);
  UNIT_TEST_ASSERT(list_length(lst) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_stack, "Stack Push/Pop");
UNIT_TEST(test_stack)
{
  STACK(stack);

  UNIT_TEST_BEGIN();

  memset(elements, 0, sizeof(elements));
  stack_init(stack);

  /* Starts from empty */
  UNIT_TEST_ASSERT(stack_is_empty(stack) == true);
  UNIT_TEST_ASSERT(stack_peek(stack) == NULL);
  UNIT_TEST_ASSERT(stack_pop(stack) == NULL);

  /*
   * Push two elements. Peek and pop should be the last one. Stack should be
   * non-empty after the pop
   */
  stack_push(stack, &elements[0]);
  stack_push(stack, &elements[1]);

  UNIT_TEST_ASSERT(stack_peek(stack) == &elements[1]);
  UNIT_TEST_ASSERT(stack_pop(stack) == &elements[1]);
  UNIT_TEST_ASSERT(stack_peek(stack) == &elements[0]);
  UNIT_TEST_ASSERT(stack_is_empty(stack) == false);
  UNIT_TEST_ASSERT(stack_pop(stack) == &elements[0]);

  /* Ends empty */
  UNIT_TEST_ASSERT(stack_is_empty(stack) == true);
  UNIT_TEST_ASSERT(stack_peek(stack) == NULL);
  UNIT_TEST_ASSERT(stack_pop(stack) == NULL);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_queue, "Queue Enqueue/Dequeue");
UNIT_TEST(test_queue)
{
  QUEUE(queue);

  UNIT_TEST_BEGIN();

  memset(elements, 0, sizeof(elements));
  queue_init(queue);

  /* Starts from empty */
  UNIT_TEST_ASSERT(queue_is_empty(queue) == true);
  UNIT_TEST_ASSERT(queue_peek(queue) == NULL);
  UNIT_TEST_ASSERT(queue_dequeue(queue) == NULL);

  /* Enqueue three elements. They should come out in the same order */
  queue_enqueue(queue, &elements[0]);
  queue_enqueue(queue, &elements[1]);
  queue_enqueue(queue, &elements[2]);

  UNIT_TEST_ASSERT(queue_dequeue(queue) == &elements[0]);
  UNIT_TEST_ASSERT(queue_dequeue(queue) == &elements[1]);
  UNIT_TEST_ASSERT(queue_dequeue(queue) == &elements[2]);

  /* Should be empty */
  UNIT_TEST_ASSERT(queue_is_empty(queue) == true);
  UNIT_TEST_ASSERT(queue_peek(queue) == NULL);
  UNIT_TEST_ASSERT(queue_dequeue(queue) == NULL);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_csll, "Circular, singly-linked list");
UNIT_TEST(test_csll)
{
  demo_struct_t *head, *tail;

  CIRCULAR_LIST(csll);

  UNIT_TEST_BEGIN();

  memset(elements, 0, sizeof(elements));
  circular_list_init(csll);

  /* Starts from empty */
  UNIT_TEST_ASSERT(circular_list_is_empty(csll) == true);
  UNIT_TEST_ASSERT(circular_list_length(csll) == 0);
  UNIT_TEST_ASSERT(circular_list_head(csll) == NULL);
  UNIT_TEST_ASSERT(circular_list_tail(csll) == NULL);

  /* Add one element. Should point to itself and act as head and tail */
  circular_list_add(csll, &elements[0]);

  UNIT_TEST_ASSERT(circular_list_is_empty(csll) == false);
  UNIT_TEST_ASSERT(circular_list_length(csll) == 1);
  UNIT_TEST_ASSERT(circular_list_head(csll) == &elements[0]);
  UNIT_TEST_ASSERT(circular_list_tail(csll) == &elements[0]);
  UNIT_TEST_ASSERT(elements[0].next == &elements[0]);

  /* Add a second element. The two should point to each-other */
  circular_list_add(csll, &elements[1]);
  UNIT_TEST_ASSERT(elements[0].next == &elements[1]);
  UNIT_TEST_ASSERT(elements[1].next == &elements[0]);

  /*
   * Add a third element and check that head->next->next points to tail.
   * Check that tail->next points to the head
   */
  circular_list_add(csll, &elements[2]);
  head = circular_list_head(csll);
  tail = circular_list_tail(csll);

  UNIT_TEST_ASSERT(head->next->next == circular_list_tail(csll));
  UNIT_TEST_ASSERT(tail->next == circular_list_head(csll));

  /* Re-add an existing element. Check the list's integrity */
  circular_list_add(csll, &elements[1]);
  head = circular_list_head(csll);
  tail = circular_list_tail(csll);

  UNIT_TEST_ASSERT(circular_list_is_empty(csll) == false);
  UNIT_TEST_ASSERT(circular_list_length(csll) == 3);
  UNIT_TEST_ASSERT(head->next->next->next == circular_list_head(csll));
  UNIT_TEST_ASSERT(head->next->next == circular_list_tail(csll));
  UNIT_TEST_ASSERT(tail->next == circular_list_head(csll));

  /* Add another two elements, then start testing removal */
  circular_list_add(csll, &elements[3]);
  circular_list_add(csll, &elements[4]);

  /* Remove an item in the middle and test list integrity */
  head = circular_list_head(csll);
  circular_list_remove(csll, head->next->next);
  head = circular_list_head(csll);
  tail = circular_list_tail(csll);

  UNIT_TEST_ASSERT(circular_list_length(csll) == 4);
  UNIT_TEST_ASSERT(head->next->next->next->next == circular_list_head(csll));
  UNIT_TEST_ASSERT(head->next->next->next == circular_list_tail(csll));
  UNIT_TEST_ASSERT(tail->next == circular_list_head(csll));

  /* Remove the head and test list integrity */
  circular_list_remove(csll, circular_list_head(csll));
  head = circular_list_head(csll);
  tail = circular_list_tail(csll);

  UNIT_TEST_ASSERT(circular_list_length(csll) == 3);
  UNIT_TEST_ASSERT(head->next->next->next == circular_list_head(csll));
  UNIT_TEST_ASSERT(head->next->next == circular_list_tail(csll));
  UNIT_TEST_ASSERT(tail->next == circular_list_head(csll));

  /* Remove the tail and test list integrity */
  circular_list_remove(csll, circular_list_tail(csll));
  head = circular_list_head(csll);
  tail = circular_list_tail(csll);

  UNIT_TEST_ASSERT(circular_list_length(csll) == 2);
  UNIT_TEST_ASSERT(head->next->next == circular_list_head(csll));
  UNIT_TEST_ASSERT(head->next == circular_list_tail(csll));
  UNIT_TEST_ASSERT(tail->next == circular_list_head(csll));

  /*
   * Remove the tail
   * Only one item left: Make sure the head and tail are the same and point to
   * each other
   */
  circular_list_remove(csll, circular_list_tail(csll));
  head = circular_list_head(csll);
  tail = circular_list_tail(csll);

  UNIT_TEST_ASSERT(circular_list_length(csll) == 1);
  UNIT_TEST_ASSERT(head == tail);
  UNIT_TEST_ASSERT(head->next->next == circular_list_head(csll));
  UNIT_TEST_ASSERT(head->next == circular_list_head(csll));
  UNIT_TEST_ASSERT(head->next == circular_list_tail(csll));
  UNIT_TEST_ASSERT(tail->next == circular_list_head(csll));
  UNIT_TEST_ASSERT(tail->next == circular_list_tail(csll));

  /* Remove the last element by removing the head */
  circular_list_remove(csll, circular_list_head(csll));
  UNIT_TEST_ASSERT(circular_list_is_empty(csll) == true);
  UNIT_TEST_ASSERT(circular_list_length(csll) == 0);
  UNIT_TEST_ASSERT(circular_list_head(csll) == NULL);
  UNIT_TEST_ASSERT(circular_list_tail(csll) == NULL);

  /* Remove the last element by removing the tail */
  circular_list_add(csll, &elements[0]);
  circular_list_remove(csll, circular_list_tail(csll));
  UNIT_TEST_ASSERT(circular_list_is_empty(csll) == true);
  UNIT_TEST_ASSERT(circular_list_length(csll) == 0);
  UNIT_TEST_ASSERT(circular_list_head(csll) == NULL);
  UNIT_TEST_ASSERT(circular_list_tail(csll) == NULL);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_dll, "Doubly-linked list");
UNIT_TEST(test_dll)
{
  demo_struct_t *head, *tail;

  CIRCULAR_LIST(dll);

  UNIT_TEST_BEGIN();

  memset(elements, 0, sizeof(elements));

  /* Starts from empty */
  dbl_list_init(dll);
  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == true);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 0);
  UNIT_TEST_ASSERT(dbl_list_head(dll) == NULL);
  UNIT_TEST_ASSERT(dbl_list_tail(dll) == NULL);

  /*
   * Add an item by adding to the head.
   * Head and tail should point to NULL in both directions
   */
  dbl_list_add_head(dll, &elements[0]);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 1);
  UNIT_TEST_ASSERT(head == &elements[0]);
  UNIT_TEST_ASSERT(tail == &elements[0]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(head->next == NULL);
  UNIT_TEST_ASSERT(tail->previous == NULL);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Add an item by adding to the tail.
   * Head and tail should point to NULL in both directions
   */
  dbl_list_remove(dll, dbl_list_head(dll));
  dbl_list_add_tail(dll, &elements[1]);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 1);
  UNIT_TEST_ASSERT(head == &elements[1]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(head->next == NULL);
  UNIT_TEST_ASSERT(tail->previous == NULL);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Add a second item to head. Head points forward to tail.
   * Tail points backwards to head.
   */
  dbl_list_add_head(dll, &elements[2]);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 2);
  UNIT_TEST_ASSERT(head == &elements[2]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(head->next == tail);
  UNIT_TEST_ASSERT(tail->previous == head);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Add before head.
   * NULL <-- 3 --> 2 --> 1 --> NULL
   */
  dbl_list_add_before(dll, dbl_list_head(dll), &elements[3]);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 3);
  UNIT_TEST_ASSERT(head == &elements[3]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(head->next == &elements[2]);
  UNIT_TEST_ASSERT(head->next->next == tail);
  UNIT_TEST_ASSERT(head->next->next->next == NULL);
  UNIT_TEST_ASSERT(tail->previous == &elements[2]);
  UNIT_TEST_ASSERT(tail->previous->previous == &elements[3]);
  UNIT_TEST_ASSERT(tail->previous->previous->previous == NULL);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Add after head.
   * NULL <-- 3 --> 4 --> 2 --> 1 --> NULL
   */
  dbl_list_add_after(dll, dbl_list_head(dll), &elements[4]);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 4);
  UNIT_TEST_ASSERT(head == &elements[3]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(head->next == &elements[4]);
  UNIT_TEST_ASSERT(head->next->next == &elements[2]);
  UNIT_TEST_ASSERT(head->next->next->next == tail);
  UNIT_TEST_ASSERT(head->next->next->next->next == NULL);
  UNIT_TEST_ASSERT(tail->previous == &elements[2]);
  UNIT_TEST_ASSERT(tail->previous->previous == &elements[4]);
  UNIT_TEST_ASSERT(tail->previous->previous->previous == &elements[3]);
  UNIT_TEST_ASSERT(tail->previous->previous->previous->previous == NULL);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Add at 3rd position by adding after 2nd
   * NULL <-- 3 --> 4 --> 5 --> 2 --> 1 --> NULL
   */
  dbl_list_add_after(dll, &elements[4], &elements[5]);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 5);
  UNIT_TEST_ASSERT(head == &elements[3]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(head->next == &elements[4]);
  UNIT_TEST_ASSERT(head->next->next == &elements[5]);
  UNIT_TEST_ASSERT(tail->previous->previous == &elements[5]);
  UNIT_TEST_ASSERT(tail->previous == &elements[2]);
  UNIT_TEST_ASSERT(elements[5].next == &elements[2]);
  UNIT_TEST_ASSERT(elements[5].previous == &elements[4]);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Add at 3rd position by adding before 3rd
   * NULL <-- 3 --> 4 --> 6 --> 5 --> 2 --> 1 --> NULL
   */
  dbl_list_add_before(dll, &elements[5], &elements[6]);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 6);
  UNIT_TEST_ASSERT(head == &elements[3]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(head->next == &elements[4]);
  UNIT_TEST_ASSERT(head->next->next == &elements[6]);
  UNIT_TEST_ASSERT(tail->previous->previous == &elements[5]);
  UNIT_TEST_ASSERT(elements[6].next == &elements[5]);
  UNIT_TEST_ASSERT(elements[6].previous == &elements[4]);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Add before tail
   * NULL <-- 3 --> 4 --> 6 --> 5 --> 2 --> 7 --> 1 --> NULL
   */
  dbl_list_add_before(dll, dbl_list_tail(dll), &elements[7]);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 7);
  UNIT_TEST_ASSERT(head == &elements[3]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(tail->previous == &elements[7]);
  UNIT_TEST_ASSERT(elements[7].next == &elements[1]);
  UNIT_TEST_ASSERT(elements[7].previous == &elements[2]);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Add after tail
   * NULL <-- 3 --> 4 --> 6 --> 5 --> 2 --> 7 --> 1 --> 8 --> NULL
   */
  dbl_list_add_after(dll, dbl_list_tail(dll), &elements[8]);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 8);
  UNIT_TEST_ASSERT(head == &elements[3]);
  UNIT_TEST_ASSERT(tail == &elements[8]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(tail->previous == &elements[1]);
  UNIT_TEST_ASSERT(elements[8].next == NULL);
  UNIT_TEST_ASSERT(elements[8].previous == &elements[1]);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Find and remove element 5
   * NULL <-- 3 --> 4 --> 6 --> 2 --> 7 --> 1 --> 8 --> NULL
   */
  dbl_list_remove(dll, &elements[5]);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 7);
  UNIT_TEST_ASSERT(head == &elements[3]);
  UNIT_TEST_ASSERT(tail == &elements[8]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(elements[6].next == &elements[2]);
  UNIT_TEST_ASSERT(elements[2].previous == &elements[6]);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Remove before tail
   * NULL <-- 3 --> 4 --> 6 --> 2 --> 7 --> 8 --> NULL
   */
  dbl_list_remove(dll, ((demo_struct_t *)dbl_list_tail(dll))->previous);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 6);
  UNIT_TEST_ASSERT(head == &elements[3]);
  UNIT_TEST_ASSERT(tail == &elements[8]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(elements[7].next == tail);
  UNIT_TEST_ASSERT(tail->previous == &elements[7]);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Remove after head
   * NULL <-- 3 --> 6 --> 2 --> 7 --> 8 --> NULL
   */
  dbl_list_remove(dll, ((demo_struct_t *)dbl_list_head(dll))->next);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 5);
  UNIT_TEST_ASSERT(head == &elements[3]);
  UNIT_TEST_ASSERT(tail == &elements[8]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(head->next == &elements[6]);
  UNIT_TEST_ASSERT(elements[6].previous == head);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Find element 2 and remove whatever is after it
   * NULL <-- 3 --> 6 --> 2 --> 8 --> NULL
   */
  dbl_list_remove(dll, elements[2].next);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 4);
  UNIT_TEST_ASSERT(head == &elements[3]);
  UNIT_TEST_ASSERT(tail == &elements[8]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(elements[2].next == tail);
  UNIT_TEST_ASSERT(tail->previous == &elements[2]);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Find element 2 and remove whatever is before it
   * NULL <-- 3 --> 2 --> 8 --> NULL
   */
  dbl_list_remove(dll, elements[2].previous);
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 3);
  UNIT_TEST_ASSERT(head == &elements[3]);
  UNIT_TEST_ASSERT(tail == &elements[8]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(head->next == &elements[2]);
  UNIT_TEST_ASSERT(elements[2].previous == head);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Remove head
   * NULL <-- 2 --> 8 --> NULL
   */
  dbl_list_remove(dll, dbl_list_head(dll));
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 2);
  UNIT_TEST_ASSERT(head == &elements[2]);
  UNIT_TEST_ASSERT(tail == &elements[8]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(head->next == tail);
  UNIT_TEST_ASSERT(tail->previous == head);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /*
   * Remove tail
   * NULL <-- 8 --> NULL
   */
  dbl_list_remove(dll, dbl_list_head(dll));
  head = dbl_list_head(dll);
  tail = dbl_list_tail(dll);

  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == false);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 1);
  UNIT_TEST_ASSERT(head == &elements[8]);
  UNIT_TEST_ASSERT(tail == &elements[8]);
  UNIT_TEST_ASSERT(head->previous == NULL);
  UNIT_TEST_ASSERT(head->next == NULL);
  UNIT_TEST_ASSERT(tail->previous == NULL);
  UNIT_TEST_ASSERT(tail->next == NULL);

  /* Remove the last element */
  dbl_list_remove(dll, dbl_list_head(dll));
  UNIT_TEST_ASSERT(dbl_list_is_empty(dll) == true);
  UNIT_TEST_ASSERT(dbl_list_length(dll) == 0);
  UNIT_TEST_ASSERT(dbl_list_head(dll) == NULL);
  UNIT_TEST_ASSERT(dbl_list_tail(dll) == NULL);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_cdll, "Circular, doubly-linked list");
UNIT_TEST(test_cdll)
{
  demo_struct_t *head, *tail;

  CIRCULAR_LIST(cdll);

  UNIT_TEST_BEGIN();

  memset(elements, 0, sizeof(elements));

  /* Starts from empty */
  dbl_circ_list_init(cdll);
  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == true);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 0);
  UNIT_TEST_ASSERT(dbl_circ_list_head(cdll) == NULL);
  UNIT_TEST_ASSERT(dbl_circ_list_tail(cdll) == NULL);

  /*
   * Add an item by adding to the head.
   * Head and tail should be the same element and should point to itself in
   * both directions
   */
  dbl_circ_list_add_head(cdll, &elements[0]);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 1);
  UNIT_TEST_ASSERT(head == &elements[0]);
  UNIT_TEST_ASSERT(tail == &elements[0]);
  UNIT_TEST_ASSERT(head->previous == head);
  UNIT_TEST_ASSERT(head->next == head);
  UNIT_TEST_ASSERT(tail->previous == tail);
  UNIT_TEST_ASSERT(tail->next == tail);

  /*
   * Add an item by adding to the tail.
   * (tail) <--> 0 <--> 1 <--> (head)
   * Head should point to tail in both directions
   */
  dbl_circ_list_add_tail(cdll, &elements[1]);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 2);
  UNIT_TEST_ASSERT(head == &elements[0]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->next == tail);
  UNIT_TEST_ASSERT(tail->previous == head);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(tail->next == head);

  /*
   * Add before head.
   * (tail) <--> 2 <--> 0 <--> 1 <--> (head)
   */
  dbl_circ_list_add_before(cdll, dbl_circ_list_head(cdll), &elements[2]);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 3);
  UNIT_TEST_ASSERT(head == &elements[2]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(tail->next == head);
  UNIT_TEST_ASSERT(elements[2].previous == tail);
  UNIT_TEST_ASSERT(elements[2].next == &elements[0]);
  UNIT_TEST_ASSERT(elements[0].previous == head);

  /*
   * Add after head.
   * (tail) <--> 2 <--> 3 <--> 0 <--> 1 <--> (head)
   */
  dbl_circ_list_add_after(cdll, dbl_circ_list_head(cdll), &elements[3]);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 4);
  UNIT_TEST_ASSERT(head == &elements[2]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(tail->next == head);
  UNIT_TEST_ASSERT(elements[3].previous == head);
  UNIT_TEST_ASSERT(elements[3].next == &elements[0]);
  UNIT_TEST_ASSERT(elements[0].previous == &elements[3]);

  /*
   * Add at 3rd position by adding after 2nd
   * (tail) <--> 2 <--> 3 <--> 4 <--> 0 <--> 1 <--> (head)
   */
  dbl_circ_list_add_after(cdll, &elements[3], &elements[4]);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 5);
  UNIT_TEST_ASSERT(head == &elements[2]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(tail->next == head);
  UNIT_TEST_ASSERT(elements[3].next == &elements[4]);
  UNIT_TEST_ASSERT(elements[4].previous == &elements[3]);
  UNIT_TEST_ASSERT(elements[4].next == &elements[0]);
  UNIT_TEST_ASSERT(elements[0].previous == &elements[4]);

  /*
   * Add at 3rd position by adding before 3rd
   * (tail) <--> 2 <--> 3 <--> 5 <--> 4 <--> 0 <--> 1 <--> (head)
   */
  dbl_circ_list_add_before(cdll, &elements[4], &elements[5]);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 6);
  UNIT_TEST_ASSERT(head == &elements[2]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(tail->next == head);
  UNIT_TEST_ASSERT(elements[3].next == &elements[5]);
  UNIT_TEST_ASSERT(elements[5].previous == &elements[3]);
  UNIT_TEST_ASSERT(elements[5].next == &elements[4]);
  UNIT_TEST_ASSERT(elements[4].previous == &elements[5]);

  /*
   * Add before tail
   * (tail) <--> 2 <--> 3 <--> 5 <--> 4 <--> 0 <--> 6 <--> 1 <--> (head)
   */
  dbl_circ_list_add_before(cdll, dbl_circ_list_tail(cdll), &elements[6]);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 7);
  UNIT_TEST_ASSERT(head == &elements[2]);
  UNIT_TEST_ASSERT(tail == &elements[1]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(tail->next == head);
  UNIT_TEST_ASSERT(elements[0].next == &elements[6]);
  UNIT_TEST_ASSERT(elements[6].previous == &elements[0]);
  UNIT_TEST_ASSERT(elements[6].next == &elements[1]);
  UNIT_TEST_ASSERT(elements[1].previous == &elements[6]);

  /*
   * Add after tail
   * (tail) <--> 2 <--> 3 <--> 5 <--> 4 <--> 0 <--> 6 <--> 1 <--> 7 <--> (head)
   */
  dbl_circ_list_add_after(cdll, dbl_circ_list_tail(cdll), &elements[7]);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 8);
  UNIT_TEST_ASSERT(head == &elements[2]);
  UNIT_TEST_ASSERT(tail == &elements[7]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(tail->next == head);
  UNIT_TEST_ASSERT(elements[1].next == &elements[7]);
  UNIT_TEST_ASSERT(elements[7].previous == &elements[1]);
  UNIT_TEST_ASSERT(elements[7].next == &elements[2]);
  UNIT_TEST_ASSERT(elements[2].previous == &elements[7]);

  /*
   * Find and remove element 5
   * (tail) <--> 2 <--> 3 <--> 4 <--> 0 <--> 6 <--> 1 <--> 7 <--> (head)
   */
  dbl_circ_list_remove(cdll, &elements[5]);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 7);
  UNIT_TEST_ASSERT(head == &elements[2]);
  UNIT_TEST_ASSERT(tail == &elements[7]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(tail->next == head);
  UNIT_TEST_ASSERT(elements[3].next == &elements[4]);
  UNIT_TEST_ASSERT(elements[4].previous == &elements[3]);

  /*
   * Find element 4 and remove what's after it
   * (tail) <--> 2 <--> 3 <--> 4 <--> 6 <--> 1 <--> 7 <--> (head)
   */
  dbl_circ_list_remove(cdll, elements[4].next);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 6);
  UNIT_TEST_ASSERT(head == &elements[2]);
  UNIT_TEST_ASSERT(tail == &elements[7]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(tail->next == head);
  UNIT_TEST_ASSERT(elements[4].next == &elements[6]);
  UNIT_TEST_ASSERT(elements[6].previous == &elements[4]);

  /*
   * Find element 4 and remove what's before it
   * (tail) <--> 2 <--> 4 <--> 6 <--> 1 <--> 7 <--> (head)
   */
  dbl_circ_list_remove(cdll, elements[4].previous);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 5);
  UNIT_TEST_ASSERT(head == &elements[2]);
  UNIT_TEST_ASSERT(tail == &elements[7]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(tail->next == head);
  UNIT_TEST_ASSERT(elements[2].next == &elements[4]);
  UNIT_TEST_ASSERT(elements[4].previous == &elements[2]);

  /*
   * Remove before tail
   * (tail) <--> 2 <--> 4 <--> 6 <--> 7 <--> (head)
   */
  dbl_circ_list_remove(cdll,
                       ((demo_struct_t *)dbl_circ_list_tail(cdll))->previous);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 4);
  UNIT_TEST_ASSERT(head == &elements[2]);
  UNIT_TEST_ASSERT(tail == &elements[7]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(tail->next == head);
  UNIT_TEST_ASSERT(elements[6].next == &elements[7]);
  UNIT_TEST_ASSERT(elements[7].previous == &elements[6]);

  /*
   * Remove after tail
   * (tail) <--> 4 <--> 6 <--> 7 <--> (head)
   */
  dbl_circ_list_remove(cdll,
                       ((demo_struct_t *)dbl_circ_list_tail(cdll))->next);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 3);
  UNIT_TEST_ASSERT(head == &elements[4]);
  UNIT_TEST_ASSERT(tail == &elements[7]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(tail->next == head);
  UNIT_TEST_ASSERT(elements[7].next == &elements[4]);
  UNIT_TEST_ASSERT(elements[4].previous == &elements[7]);

  /*
   * Remove after head
   * (tail) <--> 4 <--> 7 <--> (head)
   */
  dbl_circ_list_remove(cdll,
                       ((demo_struct_t *)dbl_circ_list_head(cdll))->next);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 2);
  UNIT_TEST_ASSERT(head == &elements[4]);
  UNIT_TEST_ASSERT(tail == &elements[7]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(head->next == tail);
  UNIT_TEST_ASSERT(tail->previous == head);
  UNIT_TEST_ASSERT(tail->next == head);

  /*
   * Remove before head
   * (tail) <--> 4 <--> (head)
   */
  dbl_circ_list_remove(cdll,
                       ((demo_struct_t *)dbl_circ_list_head(cdll))->previous);
  head = dbl_circ_list_head(cdll);
  tail = dbl_circ_list_tail(cdll);

  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == false);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 1);
  UNIT_TEST_ASSERT(head == &elements[4]);
  UNIT_TEST_ASSERT(tail == &elements[4]);
  UNIT_TEST_ASSERT(head->previous == tail);
  UNIT_TEST_ASSERT(head->next == tail);

  /* Remove head */
  dbl_circ_list_remove(cdll, dbl_circ_list_head(cdll));
  dbl_circ_list_remove(cdll, dbl_circ_list_head(cdll));
  UNIT_TEST_ASSERT(dbl_circ_list_is_empty(cdll) == true);
  UNIT_TEST_ASSERT(dbl_circ_list_length(cdll) == 0);
  UNIT_TEST_ASSERT(dbl_circ_list_head(cdll) == NULL);
  UNIT_TEST_ASSERT(dbl_circ_list_tail(cdll) == NULL);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(data_structure_test_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  memset(elements, 0, sizeof(elements));

  UNIT_TEST_RUN(test_list);
  UNIT_TEST_RUN(test_stack);
  UNIT_TEST_RUN(test_queue);
  UNIT_TEST_RUN(test_csll);
  UNIT_TEST_RUN(test_dll);
  UNIT_TEST_RUN(test_cdll);

  printf("=check-me= DONE\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

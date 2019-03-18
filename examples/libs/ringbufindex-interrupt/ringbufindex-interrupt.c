/*
 * Copyright (C) 2019, Toshiba Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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

#include <contiki.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/rtimer.h>
#include <sys/clock.h>
#include <lib/ringbufindex.h>
#include <dev/watchdog.h>

#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE "rbufindex"
#include <sys/log.h>

/*---------------------------------------------------------------------------*/
typedef uint16_t msg_t;

/**
 * Deliberately take time to move a message from source to dest. It
 * copies the message one bit at a time.
 */
static inline
void msg_move(msg_t *dest, const msg_t *source)
{
  int bit_index;
  *dest = 0;
  for(bit_index = 0 ; bit_index < sizeof(msg_t) * 8 ; bit_index++) {
    msg_t mask = 1 << bit_index;
    int wait;
    *dest = (*dest & (~mask)) | (*source & mask);
    for(wait = 0 ; wait < MOVE_WAIT_COUNT ; wait++) {
      /* emulate long access time... */
      ;
    }
  }
}

/*---------------------------------------------------------------------------*/

static msg_t queue[QUEUE_LEN];
static struct ringbufindex queue_rb;

/*---------------------------------------------------------------------------*/
struct message_gen {
  msg_t next;
  uint32_t generated_num;
};

static msg_t
message_gen_next(struct message_gen *gen)
{
  msg_t n = gen->next;
  gen->next++;
  gen->generated_num++;
  return n;
}
/*---------------------------------------------------------------------------*/
struct message_store {
  msg_t *storage;
  uint32_t current_num;
};

static void
message_store_input(struct message_store *store, const msg_t *val)
{
  msg_move(&store->storage[store->current_num], val);
  store->current_num++;
}

/*---------------------------------------------------------------------------*/

#define MESSAGE_POOL_SIZE (NORMAL_GET_NUM + INTERRUPT_GET_NUM)
static msg_t message_pool[MESSAGE_POOL_SIZE];

static struct message_store normal_store = {message_pool, 0};
static struct message_store interrupt_store = {message_pool + NORMAL_GET_NUM, 0};

/*---------------------------------------------------------------------------*/

#if PUT_USE_PEEK
static inline
int do_put(struct message_gen *gen)
{
  int i = ringbufindex_peek_put(&queue_rb);
  msg_t next;
  if(i < 0) return 0;
  next = message_gen_next(gen);
  msg_move(&queue[i], &next);
  ringbufindex_put(&queue_rb);
  return 1;
}
#else /* PUT_USE_PEEK */
static inline
int do_put(struct message_gen *gen)
{
  int i = ringbufindex_atomic_put(&queue_rb);
  msg_t next;
  if(i < 0) return 0;
  next = message_gen_next(gen);
  msg_move(&queue[i], &next);
  return 1;
}
#endif /* PUT_USE_PEEK */

#if GET_USE_PEEK
static inline
int do_get(struct message_store *store)
{
  int i = ringbufindex_peek_get(&queue_rb);
  if(i < 0) return 0;
  message_store_input(store, &queue[i]);
  ringbufindex_get(&queue_rb);
  return 1;
}
#else /* GET_USE_PEEK */
static inline
int do_get(struct message_store *store)
{
  int i = ringbufindex_atomic_get(&queue_rb);
  if(i < 0) return 0;
  message_store_input(store, &queue[i]);
  return 1;
}
#endif /* GET_USE_PEEK */

/*---------------------------------------------------------------------------*/
static struct message_gen normal_put_gen = {0, 0};
static struct message_gen interrupt_put_gen = {NORMAL_PUT_NUM, 0};
static int do_put_in_rtimer = 0;
static int finished_normal_put = (NORMAL_PUT_NUM == 0);
static int finished_normal_get = (NORMAL_GET_NUM == 0);
static volatile int finished_interrupt_put = (INTERRUPT_PUT_NUM == 0);
static volatile int finished_interrupt_get = (INTERRUPT_GET_NUM == 0);

static void task_rtimer(struct rtimer *rt, void *data);

static void
schedule_rtimer(void)
{
  static struct rtimer rt;
  rtimer_set(&rt, RTIMER_NOW() + INTERRUPT_RTIMER_INTERVAL, 0, &task_rtimer, NULL);
}

static void
task_rtimer(struct rtimer *rt, void *data)
{
  if(do_put_in_rtimer) {
    if(interrupt_put_gen.generated_num < INTERRUPT_PUT_NUM) {
      if(do_put(&interrupt_put_gen)) {
        LOG_DBG("Interrupt put\n");
      }
      if(interrupt_put_gen.generated_num == INTERRUPT_PUT_NUM) {
        finished_interrupt_put = 1;
        LOG_INFO("Interrupt put done\n");
      }
    }
  } else {
    if(interrupt_store.current_num < INTERRUPT_GET_NUM) {
      if(do_get(&interrupt_store)) {
        LOG_DBG("Interrupt get\n");
      }
      if(interrupt_store.current_num == INTERRUPT_GET_NUM) {
        finished_interrupt_get = 1;
        LOG_INFO("Interrupt get done\n");
      }
    }
  }
  do_put_in_rtimer = !do_put_in_rtimer;
  if(!finished_interrupt_put || !finished_interrupt_get) {
    schedule_rtimer();
  } else {
    LOG_INFO("Finished rtimer\n");
  }
}
/*---------------------------------------------------------------------------*/
static int
message_comparator(const void *a, const void *b)
{
  const msg_t *msg_a = (const msg_t *)a;
  const msg_t *msg_b = (const msg_t *)b;
  return
    (*msg_a < *msg_b) ? -1
    : (*msg_a > *msg_b) ? 1
    : 0;
}

static void
check_result(void)
{
  int index = 0;
  int is_ok = 1;
  qsort(message_pool, MESSAGE_POOL_SIZE, sizeof(msg_t), message_comparator);
  for(index = 0 ; index < NORMAL_GET_NUM + INTERRUPT_GET_NUM ; index++) {
    if(message_pool[index] != (msg_t)index) {
      is_ok = 0;
      LOG_ERR("message_pool[%d] = %u (should be %u)\n", index, message_pool[index], (msg_t)index);
    }
    watchdog_periodic();
  }
  for( ; index < MESSAGE_POOL_SIZE ; index++) {
    if(message_pool[index] != 0xFFFF) {
      is_ok = 0;
      LOG_ERR("message_pool[%d] = %u (should be %u)\n", index, message_pool[index], 0xFF);
    }
    watchdog_periodic();
  }
  if(ringbufindex_elements(&queue_rb) != 0) {
    is_ok = 0;
    LOG_ERR("%d elements still in the queue (should be 0)\n", ringbufindex_elements(&queue_rb));
  }
  LOG_INFO("Check result: %s\n", is_ok ? "OK" : "NG");
}
/*---------------------------------------------------------------------------*/
PROCESS(normal_put, "normal PUT process");
PROCESS_THREAD(normal_put, ev, data)
{
  PROCESS_BEGIN();
  while(!finished_normal_put) {
    PROCESS_PAUSE();
    if(normal_put_gen.generated_num < NORMAL_PUT_NUM) {
      if(do_put(&normal_put_gen)) {
        LOG_DBG("Normal put\n");
      }
      if(normal_put_gen.generated_num == NORMAL_PUT_NUM) {
        finished_normal_put = 1;
        LOG_INFO("Normal put done\n");
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS(normal_get, "normal GET process");
PROCESS_THREAD(normal_get, ev, data)
{
  PROCESS_BEGIN();
  while(!finished_normal_get) {
    PROCESS_PAUSE();
    if(normal_store.current_num < NORMAL_GET_NUM) {
      if(do_get(&normal_store)) {
        LOG_DBG("Normal get\n");
      }
      if(normal_store.current_num == NORMAL_GET_NUM) {
        finished_normal_get = 1;
        LOG_INFO("Normal get done\n");
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS(main_process, "the main process");
AUTOSTART_PROCESSES(&main_process);
PROCESS_THREAD(main_process, ev, data)
{
  PROCESS_BEGIN();
  LOG_INFO("ringbufindex-interrupt started.\n");
  LOG_INFO("QUEUE_LEN = %d\n", QUEUE_LEN);
  LOG_INFO("NORMAL_PUT_NUM = %d\n", NORMAL_PUT_NUM);
  LOG_INFO("INTERRUPT_PUT_NUM = %d\n", INTERRUPT_PUT_NUM);
  LOG_INFO("NORMAL_GET_NUM = %d\n", NORMAL_GET_NUM);
  LOG_INFO("INTERRUPT_GET_NUM = %d\n", INTERRUPT_GET_NUM);
  LOG_INFO("PUT_USE_PEEK = %d\n", PUT_USE_PEEK);
  LOG_INFO("GET_USE_PEEK = %d\n", GET_USE_PEEK);
  LOG_INFO("INTERRUPT_RTIMER_INTERVAL = %ld\n", INTERRUPT_RTIMER_INTERVAL);

  memset(message_pool, 0xFF, sizeof(message_pool));
  ringbufindex_init(&queue_rb, QUEUE_LEN);
  schedule_rtimer();
  process_start(&normal_get, NULL);
  process_start(&normal_put, NULL);
  PROCESS_PAUSE();
  
  while(1) {
    PROCESS_PAUSE();
    if(finished_interrupt_put && finished_normal_put) {
      if(finished_interrupt_get && finished_normal_get) {
        /* PUT and GET all finished. */
        break;
      }
      if(ringbufindex_empty(&queue_rb)) {
        /* Due to some bugs in ringbufindex, it's possible that
         * messages are drained even if the consumers are not finished
         * yet.
         */
        break;
      }
    }
  }
  check_result();
  LOG_INFO("Now resetting..\n");
  watchdog_reboot();
  
  PROCESS_END();
}

/*
 * Copyright (c) 2015, SICS Swedish ICT.
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
 *         ringbufindex library. Implements basic support for ring buffers
 *         of any type, as opposed to the os/lib/ringbuf module which
 *         is only for byte arrays. Simply returns index in the ringbuf
 *         rather than actual elements. The ringbuf size must be power of two.
 *         Like the original ringbuf, this module implements atomic put and get.
 *
 *         ## Put/get API and its interrupt-safety
 *
 *         ringbufindex has two types APIs for put and get operations: the
 *         "peek" API and "atomic" APIs. They have different properties on
 *         interrupt-safety. This section provides a guideline about which API
 *         you should use.
 *
 *         With the "peek" API, you peek the next index from the ringbufindex,
 *         access the actual data and commit the data.
 *
 *             i = ringbufindex_peek_put(&rb);
 *             if(i >= 0) {
 *                 store_data_into(&queue[i]);
 *                 ringbufindex_put(&rb);  // commit
 *             }
 *
 *         With the "atomic" API, you retrieve the next index from the
 *         ringbufindex atomically and access the actual data.
 *
 *             i = ringbufindex_atomic_put(&rb);
 *             if(i >= 0) {
 *                 store_data_into(&queue[i]);
 *             }
 *
 *         The two APIs are defined both for put and get operations.
 *
 *         ### (put -> get, get -> put) => get: peek, put: peek
 *
 *         If put operation can interrupt get operation and/or get operation can
 *         interrupt put operation (but there is no other interruption cases),
 *         you should use the "peek" API both for get and put. The "peek" API
 *         ensures no data is popped or overwritten prematurely.
 *
 *         ### (put -> get, put -> put) => get: peek, put: atomic
 *
 *         If put operation can interrupt both put and get operations (but there
 *         is no other interruption cases), you should use the "peek" API for
 *         get and "atomic" API for put. The "atomic" operation arbitrates its
 *         parallel executions, so that they deal with different indices.
 *
 *         ### (get -> put, get -> get) => get: atomic, put: peek
 *
 *         Dual of the above case.
 *
 *         ### Other cases => Not supported
 *
 *         Currently ringbufindex does not support other interruption cases,
 *         such as (put -> get, put -> put, get -> put, get -> get).
 *
 *         ### See also
 *
 *         Lock-Free Multi-Producer Multi-Consumer Queue on Ring Buffer
 *         https://www.linuxjournal.com/content/lock-free-multi-producer-multi-consumer-queue-ring-buffer
 *
 *
 * \author
 *         Simon Duquennoy <simonduq@sics.se>
 *         based on Contiki's os/lib/ringbuf library by Adam Dunkels
 */

#include <string.h>
#include "sys/atomic.h"
#include "sys/memory-barrier.h"
#include "lib/ringbufindex.h"

static inline int
is_full_at(const struct ringbufindex *r, uint8_t temp_put_ptr)
{
  return ((temp_put_ptr - r->get_ptr) & r->mask) == r->mask;
}

static inline int
is_full(const struct ringbufindex *r)
{
  return is_full_at(r, r->put_ptr);
}

static inline int
is_empty_at(const struct ringbufindex *r, uint8_t temp_get_ptr)
{
  return ((r->put_ptr - temp_get_ptr) & r->mask) == 0;
}

static inline int
is_empty(const struct ringbufindex *r)
{
  return is_empty_at(r, r->get_ptr);
}

/* Initialize a ring buffer. The size must be a power of two */
void
ringbufindex_init(struct ringbufindex *r, uint8_t size)
{
  r->mask = size - 1;
  r->put_ptr = 0;
  r->get_ptr = 0;
}
/* Put one element to the ring buffer */
int
ringbufindex_put(struct ringbufindex *r)
{
  /* Check if buffer is full. If it is full, return 0 to indicate that
   * the element was not inserted.
   */
  if(is_full(r)) {
    return 0;
  }
  r->put_ptr = (r->put_ptr + 1) & r->mask;
  return 1;
}
/* Check if there is space to put an element.
 * Return the index where the next element is to be added */
int
ringbufindex_peek_put(const struct ringbufindex *r)
{
  /* Check if there are bytes in the buffer. If so, we return the
   * first one. If there are no bytes left, we return -1.
   */
  if(is_full(r)) {
    return -1;
  }
  return r->put_ptr;
}

int
ringbufindex_atomic_put(struct ringbufindex *r)
{
  uint8_t now_put, next_put;
  while(1) {
    now_put = r->put_ptr;
    next_put = (now_put + 1) & r->mask;
    memory_barrier();
    if(is_full_at(r, now_put)) {
      return -1;
    }
    if(atomic_cas_uint8(&r->put_ptr, now_put, next_put)) {
      break;
    }
  }
  return now_put;
}
/* Remove the first element and return its index */
int
ringbufindex_get(struct ringbufindex *r)
{
  int get_ptr;

  /* Check if there are bytes in the buffer. If so, we return the
   * first one and increase the pointer. If there are no bytes left, we
   * return -1.
   */
  if(is_empty(r)) {
    return -1;
  }
  get_ptr = r->get_ptr;
  r->get_ptr = (r->get_ptr + 1) & r->mask;
  return get_ptr;
}
/* Return the index of the first element
 * (which will be removed if calling ringbufindex_peek) */
int
ringbufindex_peek_get(const struct ringbufindex *r)
{
  /* Check if there are bytes in the buffer. If so, we return the
   * first one. If there are no bytes left, we return -1.
   */
  if(is_empty(r)) {
    return -1;
  }
  return r->get_ptr;
}

int
ringbufindex_atomic_get(struct ringbufindex *r)
{
  uint8_t now_get, next_get;
  while(1) {
    now_get = r->get_ptr;
    next_get = (now_get + 1) & r->mask;
    memory_barrier();
    if(is_empty_at(r, now_get)) {
      return -1;
    }
    if(atomic_cas_uint8(&r->get_ptr, now_get, next_get)) {
      break;
    }
  }
  return now_get;
}
/* Return the ring buffer size */
int
ringbufindex_size(const struct ringbufindex *r)
{
  return r->mask + 1;
}
/* Return the number of elements currently in the ring buffer */
int
ringbufindex_elements(const struct ringbufindex *r)
{
  return (r->put_ptr - r->get_ptr) & r->mask;
}
/* Is the ring buffer full? */
int
ringbufindex_full(const struct ringbufindex *r)
{
  return is_full(r);
}
/* Is the ring buffer empty? */
int
ringbufindex_empty(const struct ringbufindex *r)
{
  return ringbufindex_elements(r) == 0;
}

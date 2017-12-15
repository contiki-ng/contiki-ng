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
 * \addtogroup data
 * @{
 *
 * \defgroup queue Queue library
 *
 * This library provides functions for the creation and manipulation of
 * queues. The library is implemented as a wrapper around the list library.
 *
 * A queue is declared using the QUEUE macro. Queue elements must be
 * allocated by the calling code and must be of a C struct datatype. In this
 * struct, the first field must be a pointer called \e next. This field will
 * be used by the library to maintain the queue. Application code must not
 * modify this field directly.
 * @{
 */
/*---------------------------------------------------------------------------*/
#ifndef QUEUE_H_
#define QUEUE_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/list.h"

#include <stdbool.h>
/*---------------------------------------------------------------------------*/
/**
 * \brief The queue data type
 */
typedef list_t queue_t;
/*---------------------------------------------------------------------------*/
/**
 * \brief Define a queue.
 *
 * This macro defines a queue.
 *
 * The datatype for elements must be a C struct. The struct's first member must
 * be a pointer called \e next. This is used internally by the library to
 * maintain data structure integrity and must not be modified directly by
 * application code.
 *
 * \param name The name of the queue.
 */
#define QUEUE(name) LIST(name)
/*---------------------------------------------------------------------------*/
struct queue {
  struct queue *next;
};
/*---------------------------------------------------------------------------*/
/**
 * \brief Initialise a queue
 * \param queue The queue
 */
static inline void
queue_init(queue_t queue)
{
  list_init(queue);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Adds an element to the tail of the queue
 * \param queue The queue
 * \param element A pointer to the element to be added
 */
static inline void
queue_enqueue(queue_t queue, void *element)
{
  list_add(queue, element);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Removes the element at the front of the queue
 * \param queue The queue
 * \return A pointer to the element removed
 *
 * If this function returns NULL if the queue was empty (queue underflow)
 */
static inline void *
queue_dequeue(queue_t queue)
{
  return list_pop(queue);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Returns the front element of the queue, without removing it
 * \param queue The queue
 * \return A pointer to the element at the front of the queue
 */
static inline void *
queue_peek(queue_t queue)
{
  return list_head(queue);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Check if a queue is empty
 * \param queue The queue
 * \retval true The queue is empty
 * \retval false The queue has at least one element
 */
static inline bool
queue_is_empty(queue_t queue)
{
  return *queue == NULL ? true : false;
}
/*---------------------------------------------------------------------------*/
#endif /* QUEUE_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

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
 * \defgroup stack-data-structure Stack library
 *
 * This library provides functions for the creation and manipulation of
 * stacks. The library is implemented as a wrapper around the list library.
 *
 * A stack is declared using the STACK macro. Stack elements must be
 * allocated by the calling code and must be of a C struct datatype. In this
 * struct, the first field must be a pointer called \e next. This field will
 * be used by the library to maintain the stack. Application code must not
 * modify this field directly.
 *
 * This library is not safe to be used within an interrupt context.
 * @{
 */
/*---------------------------------------------------------------------------*/
#ifndef STACK_H_
#define STACK_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/list.h"

#include <stdbool.h>
/*---------------------------------------------------------------------------*/
/**
 * \brief The stack data type
 */
typedef list_t stack_t;
/*---------------------------------------------------------------------------*/
/**
 * \brief Define a stack.
 *
 * This macro defines a stack.
 *
 * The datatype for elements must be a C struct. The struct's first member must
 * be a pointer called \e next. This is used internally by the library to
 * maintain data structure integrity and must not be modified directly by
 * application code.
 *
 * \param name The name of the stack.
 */
#define STACK(name) LIST(name)
/*---------------------------------------------------------------------------*/
struct stack {
  struct stack *next;
};
/*---------------------------------------------------------------------------*/
/**
 * \brief Initialise a stack
 * \param stack The stack
 */
static inline void
stack_init(stack_t stack)
{
  list_init(stack);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Adds an element to the top of the stack
 * \param stack The stack
 * \param element A pointer to the element to be added
 */
static inline void
stack_push(stack_t stack, void *element)
{
  list_push(stack, element);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Removes the top element from the stack
 * \param stack The stack
 * \return A pointer to the element popped
 *
 * If this function returns NULL if the stack was empty (stack underflow)
 */
static inline void *
stack_pop(stack_t stack)
{
  return list_pop(stack);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Returns the top element of the stack, without popping it
 * \param stack The stack
 * \return A pointer to the element at the top of the stack
 */
static inline void *
stack_peek(stack_t stack)
{
  return list_head(stack);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Check if a stack is empty
 * \param stack The stack
 * \retval true The stack is empty
 * \retval false The stack has at least one element
 */
static inline bool
stack_is_empty(stack_t stack)
{
  return *stack == NULL ? true : false;
}
/*---------------------------------------------------------------------------*/
#endif /* STACK_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

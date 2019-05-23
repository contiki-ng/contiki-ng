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
/** \addtogroup data
 * @{
 *
 * \defgroup circular-singly-linked-list Circular, singly-linked list
 *
 * This library provides functions for the creation and manipulation of
 * circular, singly-linked lists.
 *
 * A circular, singly-linked list is declared using the CIRCULAR_LIST macro.
 * Elements must be allocated by the calling code and must be of a C struct
 * datatype. In this struct, the first field must be a pointer called \e next.
 * This field will be used by the library to maintain the list. Application
 * code must not modify this field directly.
 *
 * Functions that modify the list (add / remove) will, in the general case,
 * update the list's head and item order. If you call one of these functions
 * as part of a list traversal, it is advised to stop / restart traversing
 * after the respective function returns.
 *
 * This library is not safe to be used within an interrupt context.
 * @{
 */
/*---------------------------------------------------------------------------*/
#ifndef CIRCULAR_LIST_H_
#define CIRCULAR_LIST_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/**
 * \brief Define a circular, singly-linked list.
 *
 * This macro defines a circular, singly-linked list.
 *
 * The datatype for elements must be a C struct. The struct's first member must
 * be a pointer called \e next. This is used internally by the library to
 * maintain data structure integrity and must not be modified directly by
 * application code.
 *
 * \param name The name of the circular, singly-linked list.
 */
#define CIRCULAR_LIST(name) \
  static void *name##_circular_list = NULL; \
  static circular_list_t name = (circular_list_t)&name##_circular_list
/*---------------------------------------------------------------------------*/
/**
 * \brief The circular, singly-linked list datatype
 */
typedef void **circular_list_t;
/*---------------------------------------------------------------------------*/
/**
 * \brief Initialise a circular, singly-linked list.
 * \param cl The circular, singly-linked list.
 */
void circular_list_init(circular_list_t cl);

/**
 * \brief Return the tail of a circular, singly-linked list.
 * \param cl The circular, singly-linked list.
 * \return A pointer to the list's head, or NULL if the list is empty
 */
void *circular_list_head(circular_list_t cl);

/**
 * \brief Return the tail of a circular, singly-linked list.
 * \param cl The circular, singly-linked list.
 * \return A pointer to the list's tail, or NULL if the list is empty
 */
void *circular_list_tail(circular_list_t cl);

/**
 * \brief Add an element to a circular, singly-linked list.
 * \param cl The circular, singly-linked list.
 * \param element A pointer to the element to be added.
 *
 * The caller should make no assumptions as to the position in the list of the
 * new element.
 *
 * After this function returns, the list's head is not guaranteed to be the
 * same as it was before the addition.
 *
 * Calling this function will update the list's head and item order. If you
 * call this function as part of a list traversal, it is advised to stop
 * traversing after this function returns.
 */
void circular_list_add(circular_list_t cl, void *element);

/**
 * \brief Remove an element from a circular, singly-linked list.
 * \param cl The circular, singly-linked list.
 * \param element A pointer to the element to be removed.
 *
 * After this function returns, the list's head is not guaranteed to be the
 * same as it was before the addition.
 *
 * Calling this function will update the list's head and item order. If you
 * call this function as part of a list traversal, it is advised to stop
 * traversing after this function returns.
 */
void circular_list_remove(circular_list_t cl, void *element);

/**
 * \brief Get the length of a circular, singly-linked list.
 * \param cl The circular, singly-linked list.
 * \return The number of elements in the list
 */
unsigned long circular_list_length(circular_list_t cl);

/**
 * \brief Determine whether a circular, singly-linked list is empty.
 * \param cl The circular, singly-linked list.
 * \retval true The list is empty
 * \retval false The list is not empty
 */
bool circular_list_is_empty(circular_list_t cl);
/*---------------------------------------------------------------------------*/
#endif /* CIRCULAR_LIST_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

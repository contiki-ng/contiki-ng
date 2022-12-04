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
 * \defgroup doubly-linked-list Doubly-linked list
 *
 * This library provides functions for the creation and manipulation of
 * doubly-linked lists.
 *
 * A doubly-linked list is declared using the DBL_LIST macro.
 * Elements must be allocated by the calling code and must be of a C struct
 * datatype. In this struct, the first field must be a pointer called \e next.
 * The second field must be a pointer called \e previous.
 * These fields will be used by the library to maintain the list. Application
 * code must not modify these fields directly.
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
#ifndef DBL_LIST_H_
#define DBL_LIST_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/**
 * \brief Define a doubly-linked list.
 *
 * This macro defines a doubly-linked list.
 *
 * The datatype for elements must be a C struct.
 * The struct's first member must be a pointer called \e next.
 * The second field must be a pointer called \e previous.
 * These fields will be used by the library to maintain the list. Application
 * code must not modify these fields directly.
 *
 * \param name The name of the doubly-linked list.
 */
#define DBL_LIST(name) \
  static void *name##_dbl_list = NULL; \
  static dbl_list_t name = (dbl_list_t)&name##_dbl_list
/*---------------------------------------------------------------------------*/
/**
 * The doubly-linked list datatype.
 */
typedef void **dbl_list_t;

/**
 * The non-modifiable doubly-linked list type.
 */
typedef void *const *const_dbl_list_t;

/*---------------------------------------------------------------------------*/
/**
 * \brief Initialise a doubly-linked list.
 * \param dll The doubly-linked list.
 */
void dbl_list_init(dbl_list_t dll);

/**
 * \brief Return the tail of a doubly-linked list.
 * \param dll The doubly-linked list.
 * \return A pointer to the list's head, or NULL if the list is empty
 */
void *dbl_list_head(const_dbl_list_t dll);

/**
 * \brief Return the tail of a doubly-linked list.
 * \param dll The doubly-linked list.
 * \return A pointer to the list's tail, or NULL if the list is empty
 */
void *dbl_list_tail(const_dbl_list_t dll);

/**
 * \brief Add an element to the head of a doubly-linked list.
 * \param dll The doubly-linked list.
 * \param element A pointer to the element to be added.
 *
 * Calling this function will update the list's head and item order. If you
 * call this function as part of a list traversal, it is advised to stop
 * traversing after this function returns.
 */
void dbl_list_add_head(dbl_list_t dll, void *element);

/**
 * \brief Add an element to the tail of a doubly-linked list.
 * \param dll The doubly-linked list.
 * \param element A pointer to the element to be added.
 *
 * Calling this function will update the list's head and item order. If you
 * call this function as part of a list traversal, it is advised to stop
 * traversing after this function returns.
 */
void dbl_list_add_tail(dbl_list_t dll, void *element);

/**
 * \brief Add an element to a doubly linked list after an existing element.
 * \param dll The doubly-linked list.
 * \param existing A pointer to the existing element.
 * \param element A pointer to the element to be added.
 *
 * This function will add \e element after \e existing
 *
 * The function will not verify that \e existing is already part of the list.
 *
 * Calling this function will update the list's head and item order. If you
 * call this function as part of a list traversal, it is advised to stop
 * traversing after this function returns.
 */
void dbl_list_add_after(dbl_list_t dll, void *existing, void *element);

/**
 * \brief Add an element to a doubly linked list before an existing element.
 * \param dll The doubly-linked list.
 * \param existing A pointer to the existing element.
 * \param element A pointer to the element to be added.
 *
 * This function will add \e element before \e existing
 *
 * The function will not verify that \e existing is already part of the list.
 *
 * Calling this function will update the list's head and item order. If you
 * call this function as part of a list traversal, it is advised to stop
 * traversing after this function returns.
 */
void dbl_list_add_before(dbl_list_t dll, void *existing, void *element);

/**
 * \brief Remove an element from a doubly-linked list.
 * \param dll The doubly-linked list.
 * \param element A pointer to the element to be removed.
 *
 * Calling this function will update the list's head and item order. If you
 * call this function as part of a list traversal, it is advised to stop
 * traversing after this function returns.
 */
void dbl_list_remove(dbl_list_t dll, const void *element);

/**
 * \brief Get the length of a doubly-linked list.
 * \param dll The doubly-linked list.
 * \return The number of elements in the list
 */
unsigned long dbl_list_length(const_dbl_list_t dll);

/**
 * \brief Determine whether a doubly-linked list is empty.
 * \param dll The doubly-linked list.
 * \retval true The list is empty
 * \retval false The list is not empty
 */
bool dbl_list_is_empty(const_dbl_list_t dll);
/*---------------------------------------------------------------------------*/
#endif /* DBL_LIST_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

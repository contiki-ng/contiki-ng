/*
 * Copyright (c) 2005, Nicolas Tsiftes
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
 * 3. Neither the name of the author nor the names of the contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \addtogroup mem
 * @{
 */

/**
 * \defgroup heapmem heapmem: Dynamic heap memory allocator
 *
 * The heapmem module is a dynamic heap memory allocator similar to
 * malloc() in standard C. The heap memory is managed in a block of
 * static memory, whose size is determined at compile-time by setting
 * HEAPMEM_CONF_ARENA_SIZE parameter. By default, the heap memory is
 * only 1 bytes, which entails that this parameter must be set
 * explicitly in order to be possible to use this module.
 *
 * Each allocated memory object is referred to as a "chunk". The
 * allocator manages free chunks in a double-linked list. While this
 * adds some memory overhead compared to a single-linked list, it
 * improves the performance of list management.
 *
 * Internally, allocated chunks can be retrieved using the pointer to
 * the allocated memory returned by heapmem_alloc() and
 * heapmem_realloc(), because the chunk structure immediately precedes
 * the memory of the chunk.
 *
 * \note This module does not contain a corresponding function to the
 *       standard C function calloc().
 *
 * \note Dynamic memory should be used carefully on
 *       memory-constrained, embedded systems, because fragmentation
 *       may be induced through various allocation/deallocation
 *       patterns, and no guarantees are given regarding the
 *       availability of memory.
 *
 * @{
 */

/**
 * \file
 *         Header file for the dynamic heap memory allocator.
 * \author
 *         Nicolas Tsiftes <nvt@acm.org>
 */

#ifndef HEAPMEM_H
#define HEAPMEM_H

#include <stdlib.h>

typedef struct heapmem_stats {
  size_t allocated;
  size_t overhead;
  size_t available;
  size_t footprint;
  size_t chunks;
} heapmem_stats_t;

#if HEAPMEM_DEBUG

#define heapmem_alloc(size) heapmem_alloc_debug((size), __FILE__, __LINE__)
#define heapmem_realloc(ptr, size) heapmem_realloc_debug((ptr), (size), __FILE__, __LINE__)
#define heapmem_free(ptr) heapmem_free_debug((ptr), __FILE__, __LINE__)

void *heapmem_alloc_debug(size_t size,
			  const char *file, const unsigned line);
void *heapmem_realloc_debug(void *ptr, size_t size,
			    const char *file, const unsigned line);
void heapmem_free_debug(void *ptr,
			const char *file, const unsigned line);

#else

/**
 * \brief      Allocate a chunk of memory in the heap.
 * \param size The number of bytes to allocate.
 * \return     A pointer to the allocated memory chunk,
 *             or NULL if the allocation failed.
 *
 * \sa         heapmem_realloc
 * \sa         heapmem_free
 */

void *heapmem_alloc(size_t size);

/**
 * \brief      Reallocate a chunk of memory in the heap.
 * \param ptr  A pointer to a chunk that has been allocated using
 *             heapmem_alloc() or heapmem_realloc().
 * \param size The number of bytes to allocate.
 * \return     A pointer to the allocated memory chunk,
 *             or NULL if the allocation failed.
 *
 * \note If ptr is NULL, this function behaves the same as heapmem_alloc.
 * \note If ptr is not NULL and size is zero, the function deallocates
 *       the chunk and returns NULL.
 *
 * \sa         heapmem_alloc
 * \sa         heapmem_free
 */

void *heapmem_realloc(void *ptr, size_t size);

/**
 * \brief      Deallocate a chunk of memory.
 * \param ptr  A pointer to a chunk that has been allocated using
 *             heapmem_alloc() or heapmem_realloc().
 *
 * \note If ptr is NULL, this function will return immediately without
 *       without performing any action.
 *
 * \sa         heapmem_alloc
 * \sa         heapmem_realloc
 */

void heapmem_free(void *ptr);

#endif /* HEAMMEM_DEBUG */

/**
 * \brief       Obtain internal heapmem statistics regarding the
 *              allocated chunks.
 * \param stats A pointer to an object of type heapmem_stats_t, which
 *              will be filled when calling this function.
 *
 * This function makes it possible to gain visibility into the internal
 * structure of the heap. One can thus obtain information regarding
 * the amount of memory allocated, overhead used for memory management,
 * and the number of chunks allocated. By using this information, developers
 * can tune their software to use the heapmem allocator more efficiently.
 *
 */

void heapmem_stats(heapmem_stats_t *stats);

#endif /* !HEAPMEM_H */

/** @} */
/** @} */

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
 * the HEAPMEM_CONF_ARENA_SIZE parameter.
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
 * HeapMem zones provide independent heaps with their own static memory
 * buffers. Each zone has its own free list and usage tracking, so
 * fragmentation in one zone cannot affect another. Use
 * HEAPMEM_ZONE_DEFINE() to create a zone with a dedicated buffer.
 *
 * \note The HEAPMEM_CONF_ARENA_SIZE parameter enables the general
 * zone and the convenience macros (heapmem_alloc, heapmem_free, etc.).
 * Zone-specific functions (heapmem_zone_alloc, etc.) and the
 * HEAPMEM_ZONE_DEFINE macro are always available.
 *
 * \note Dynamic memory should be used carefully on
 *       memory-constrained embedded systems, because fragmentation
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

#include "contiki.h"
#include "sys/cc.h"

#include <stdlib.h>
/*****************************************************************************/
#ifndef HEAPMEM_DEBUG
#define HEAPMEM_DEBUG 0
#endif
/*****************************************************************************/
/* Alignment configuration -- needed in the header for HEAPMEM_ZONE_DEFINE. */
#if __STDC_VERSION__ >= 201112L
#include <stdalign.h>
#define HEAPMEM_DEFAULT_ALIGNMENT alignof(max_align_t)
#else
#define HEAPMEM_DEFAULT_ALIGNMENT sizeof(size_t)
#endif

#ifdef HEAPMEM_CONF_ALIGNMENT
#define HEAPMEM_ALIGNMENT HEAPMEM_CONF_ALIGNMENT
#else
#define HEAPMEM_ALIGNMENT HEAPMEM_DEFAULT_ALIGNMENT
#endif /* HEAPMEM_CONF_ALIGNMENT */
/*****************************************************************************/
typedef struct heapmem_stats {
  size_t allocated;
  size_t overhead;
  size_t available;
  size_t heap_usage;
  size_t max_heap_usage;
  size_t chunks;
} heapmem_stats_t;
/*****************************************************************************/
/*
 * A heapmem zone is an independent heap with its own static memory
 * buffer, free list, and usage tracking. Each zone provides true
 * memory isolation: fragmentation in one zone cannot affect another,
 * and each zone's memory is physically reserved.
 *
 * Use HEAPMEM_ZONE_DEFINE() to create a zone with its own static buffer.
 * When HEAPMEM_CONF_ARENA_SIZE is set, a general zone is available
 * internally and can be accessed by passing NULL as the zone parameter.
 */

/* Forward declaration for the free list pointer. */
struct heapmem_chunk;

typedef struct heapmem_zone {
  const char *name;
  char *heap_base;
  size_t arena_size;
  size_t heap_usage;
  size_t max_heap_usage;
  struct heapmem_chunk *free_list;
} heapmem_zone_t;

/**
 * \brief Define a zone with its own static memory buffer.
 * \param varname The variable name for the zone.
 * \param bufsize The size of the zone's memory buffer in bytes.
 *
 * This macro creates a file-scoped zone with a statically allocated
 * buffer. The zone provides an independent heap that is isolated from
 * all other zones.
 *
 * Example usage:
 *   HEAPMEM_ZONE_DEFINE(packet_zone, 4096);
 *   void *p = heapmem_zone_alloc(&packet_zone, 128);
 *   heapmem_zone_free(&packet_zone, p);
 */
#define HEAPMEM_ZONE_DEFINE(varname, bufsize)                              \
  static char varname##_buf_[bufsize] CC_ALIGN(HEAPMEM_ALIGNMENT);         \
  static heapmem_zone_t varname = {                                        \
    .name = #varname,                                                      \
    .heap_base = varname##_buf_,                                           \
    .arena_size = bufsize,                                                 \
  }
/*****************************************************************************/

/**
 * \brief      Allocate a chunk of memory in the specified zone.
 * \param zone A pointer to the zone in which to allocate the memory,
 *             or NULL to use the general zone.
 * \param size The number of bytes to allocate.
 * \return     A pointer to the allocated memory chunk,
 *             or NULL if the allocation failed.
 *
 * \sa         heapmem_zone_realloc
 * \sa         heapmem_zone_free
 */
void *heapmem_zone_alloc(heapmem_zone_t *zone, size_t size);

/**
 * \brief      Deallocate a chunk of memory in the specified zone.
 * \param zone A pointer to the zone from which the memory was allocated,
 *             or NULL to use the general zone.
 * \param ptr  A pointer to a chunk that has been allocated using
 *             heapmem_zone_alloc() or heapmem_zone_realloc().
 * \return     A boolean indicating whether the memory could be deallocated.
 *
 * \sa         heapmem_zone_alloc
 * \sa         heapmem_zone_realloc
 */
bool heapmem_zone_free(heapmem_zone_t *zone, void *ptr);

/**
 * \brief      Reallocate a chunk of memory in the specified zone.
 * \param zone A pointer to the zone in which to reallocate the memory,
 *             or NULL to use the general zone.
 * \param ptr  A pointer to a chunk that has been allocated using
 *             heapmem_zone_alloc() or heapmem_zone_realloc().
 * \param size The number of bytes to allocate.
 * \return     A pointer to the allocated memory chunk,
 *             or NULL if the allocation failed.
 *
 * \note If ptr is NULL, this function behaves the same as heapmem_zone_alloc.
 * \note If ptr is not NULL and size is zero, the function deallocates
 *       the chunk and returns NULL.
 *
 * \sa         heapmem_zone_alloc
 * \sa         heapmem_zone_free
 */
void *heapmem_zone_realloc(heapmem_zone_t *zone, void *ptr, size_t size);

/**
 * \brief       Allocate memory for a zero-initialized array in the
 *              specified zone.
 * \param zone  A pointer to the zone in which to allocate the memory,
 *              or NULL to use the general zone.
 * \param nmemb The number of elements to allocate.
 * \param size  The size of each element.
 * \return      A pointer to the allocated memory,
 *              or NULL if the allocation failed.
 *
 * \sa         heapmem_zone_alloc
 * \sa         heapmem_zone_free
 */
void *heapmem_zone_calloc(heapmem_zone_t *zone, size_t nmemb, size_t size);

#if HEAPMEM_DEBUG
void *heapmem_zone_alloc_debug(heapmem_zone_t *zone, size_t size,
                                const char *file, unsigned line);
bool heapmem_zone_free_debug(heapmem_zone_t *zone, void *ptr,
                              const char *file, unsigned line);
void *heapmem_zone_realloc_debug(heapmem_zone_t *zone, void *ptr, size_t size,
                                  const char *file, unsigned line);
void *heapmem_zone_calloc_debug(heapmem_zone_t *zone, size_t nmemb, size_t size,
                                 const char *file, unsigned line);
#define heapmem_zone_alloc(zone, size) \
  heapmem_zone_alloc_debug((zone), (size), __FILE__, __LINE__)
#define heapmem_zone_free(zone, ptr) \
  heapmem_zone_free_debug((zone), (ptr), __FILE__, __LINE__)
#define heapmem_zone_realloc(zone, ptr, size) \
  heapmem_zone_realloc_debug((zone), (ptr), (size), __FILE__, __LINE__)
#define heapmem_zone_calloc(zone, nmemb, size) \
  heapmem_zone_calloc_debug((zone), (nmemb), (size), __FILE__, __LINE__)
#endif /* HEAPMEM_DEBUG */

/**
 * \brief       Obtain internal statistics for a heapmem zone.
 * \param zone  A pointer to the zone to query.
 * \param stats A pointer to an object of type heapmem_stats_t, which
 *              will be filled when calling this function.
 */
void heapmem_zone_stats(heapmem_zone_t *zone, heapmem_stats_t *stats);

/**
 * \brief              Print debugging information for a heapmem zone.
 * \param zone         A pointer to the zone to query.
 * \param print_chunks Determines whether to print information about
 *                     all allocated chunks.
 */
void heapmem_zone_print_debug_info(heapmem_zone_t *zone, bool print_chunks);

/*****************************************************************************/
/* Convenience macros for the general zone. These provide backward
   compatibility with the non-zone API. */
#ifdef HEAPMEM_CONF_ARENA_SIZE

#define heapmem_alloc(size) \
  heapmem_zone_alloc(NULL, (size))
#define heapmem_free(ptr) \
  heapmem_zone_free(NULL, (ptr))
#define heapmem_realloc(ptr, size) \
  heapmem_zone_realloc(NULL, (ptr), (size))
#define heapmem_calloc(nmemb, size) \
  heapmem_zone_calloc(NULL, (nmemb), (size))
#define heapmem_stats(stats) \
  heapmem_zone_stats(NULL, (stats))
#define heapmem_print_debug_info(print_chunks) \
  heapmem_zone_print_debug_info(NULL, (print_chunks))

#endif /* HEAPMEM_CONF_ARENA_SIZE */

#endif /* !HEAPMEM_H */

/** @} */
/** @} */

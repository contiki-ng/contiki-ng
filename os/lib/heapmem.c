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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 * 	HeapMem: a dynamic memory allocation module for
 *      resource-constrained devices.
 * \author
 * 	Nicolas Tsiftes <nvt@acm.org>
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "contiki.h"
#include "lib/heapmem.h"
#include "sys/cc.h"

#ifdef HEAPMEM_CONF_ARENA_SIZE

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "HeapMem"
#define LOG_LEVEL LOG_LEVEL_WARN

/* The HEAPMEM_CONF_PRINTF function determines which function to use for
   printing debug information. */
#ifndef HEAPMEM_CONF_PRINTF
#include <stdio.h>
#define HEAPMEM_PRINTF printf
#else
#define HEAPMEM_PRINTF HEAPMEM_CONF_PRINTF
#endif /* !HEAPMEM_CONF_PRINTF */

/* The HEAPMEM_CONF_ARENA_SIZE parameter determines the size of the
   space that will be statically allocated in this module. */
#define HEAPMEM_ARENA_SIZE HEAPMEM_CONF_ARENA_SIZE

/*
 * The HEAPMEM_CONF_SEARCH_MAX parameter limits the time spent on
 * chunk allocation and defragmentation. The lower this number is, the
 * faster the operations become. The cost of this speedup, however, is
 * that the space overhead might increase.
 */
#ifdef HEAPMEM_CONF_SEARCH_MAX
#define CHUNK_SEARCH_MAX HEAPMEM_CONF_SEARCH_MAX
#else
#define CHUNK_SEARCH_MAX 16
#endif /* HEAPMEM_CONF_SEARCH_MAX */

/*
 * The HEAPMEM_CONF_REALLOC parameter determines whether
 * heapmem_realloc() is enabled (non-zero value) or not (zero value).
 */
#ifdef HEAPMEM_CONF_REALLOC
#define HEAPMEM_REALLOC HEAPMEM_CONF_REALLOC
#else
#define HEAPMEM_REALLOC 1
#endif /* HEAPMEM_CONF_REALLOC */

#if __STDC_VERSION__ >= 201112L
#include <stdalign.h>
#define HEAPMEM_DEFAULT_ALIGNMENT alignof(max_align_t)
#else
#define HEAPMEM_DEFAULT_ALIGNMENT sizeof(size_t)
#endif

/* The HEAPMEM_CONF_ALIGNMENT parameter determines the minimum
   alignment for allocated data. */
#ifdef HEAPMEM_CONF_ALIGNMENT
#define HEAPMEM_ALIGNMENT HEAPMEM_CONF_ALIGNMENT
#else
#define HEAPMEM_ALIGNMENT HEAPMEM_DEFAULT_ALIGNMENT
#endif /* HEAPMEM_CONF_ALIGNMENT */

#define ALIGN(size)						\
  (((size) + (HEAPMEM_ALIGNMENT - 1)) & ~(HEAPMEM_ALIGNMENT - 1))

/* Macros for chunk iteration. */
#define NEXT_CHUNK(chunk)						\
  ((chunk_t *)((char *)(chunk) + sizeof(chunk_t) + (chunk)->size))
#define IS_LAST_CHUNK(chunk)			\
  ((char *)NEXT_CHUNK(chunk) == &heap_base[heap_usage])

/* Macros for retrieving the data pointer from a chunk, and the other
   way around. */
#define GET_CHUNK(ptr)				\
  ((chunk_t *)((char *)(ptr) - sizeof(chunk_t)))
#define GET_PTR(chunk)				\
  (char *)((chunk) + 1)

/* Macros for determining the status of a chunk. */
#define CHUNK_FLAG_ALLOCATED            0x1

#define CHUNK_ALLOCATED(chunk)			\
  ((chunk)->flags & CHUNK_FLAG_ALLOCATED)
#define CHUNK_FREE(chunk)			\
  (~(chunk)->flags & CHUNK_FLAG_ALLOCATED)

/*
 * A heapmem zone denotes a logical subdivision of the heap that is
 * dedicated to a specific purpose. The concept of zones can help
 * developers to maintain various memory management strategies for
 * embedded systems (e.g., by having a fixed memory space for packet
 * buffers), yet maintains a level of dynamism offered by a
 * malloc-like API. The rest of the heap area that is not dedicated to
 * a specific zone belongs to the "GENERAL" zone, which can be used by
 * any module.
 */
struct heapmem_zone {
  const char *name;
  size_t zone_size;
  size_t allocated;
};

#ifdef HEAPMEM_CONF_MAX_ZONES
#define HEAPMEM_MAX_ZONES HEAPMEM_CONF_MAX_ZONES
#else
#define HEAPMEM_MAX_ZONES 1
#endif

#if HEAPMEM_MAX_ZONES < 1
#error At least one HeapMem zone must be configured.
#endif

static struct heapmem_zone zones[HEAPMEM_MAX_ZONES] = {
  {.name = "GENERAL", .zone_size = HEAPMEM_ARENA_SIZE}
};

/*
 * We use a double-linked list of chunks, with a slight space overhead
 * compared to a single-linked list, but with the advantage of having
 * much faster list removals.
 */
typedef struct chunk {
  struct chunk *prev;
  struct chunk *next;
  size_t size;
  uint8_t flags;
  heapmem_zone_t zone;
#if HEAPMEM_DEBUG
  const char *file;
  unsigned line;
#endif
} chunk_t;

/* All allocated space is located within a heap, which is
   statically allocated with a configurable size. */
static char heap_base[HEAPMEM_ARENA_SIZE] CC_ALIGN(HEAPMEM_ALIGNMENT);
static size_t heap_usage;
static size_t max_heap_usage;

static chunk_t *free_list;

#define IN_HEAP(ptr) ((ptr) != NULL && \
                     (char *)(ptr) >= (char *)heap_base) && \
                     ((char *)(ptr) < (char *)heap_base + heap_usage)

/* extend_space: Increases the current footprint used in the heap, and
   returns a pointer to the old end. */
static void *
extend_space(size_t size)
{
  if(size > HEAPMEM_ARENA_SIZE - heap_usage) {
    return NULL;
  }

  char *old_usage = &heap_base[heap_usage];
  heap_usage += size;
  if(heap_usage > max_heap_usage) {
    max_heap_usage = heap_usage;
  }

  return old_usage;
}

/* free_chunk: Mark a chunk as being free, and put it on the free list. */
static void
free_chunk(chunk_t * const chunk)
{
  chunk->flags &= ~CHUNK_FLAG_ALLOCATED;

  if(IS_LAST_CHUNK(chunk)) {
    /* Release the chunk back into the wilderness. */
    heap_usage -= sizeof(chunk_t) + chunk->size;
  } else {
    /* Put the chunk on the free list. */
    chunk->prev = NULL;
    chunk->next = free_list;
    if(free_list != NULL) {
      free_list->prev = chunk;
    }
    free_list = chunk;
  }
}

/* remove_chunk_from_free_list: Mark a chunk as being allocated, and
   remove it from the free list. */
static void
remove_chunk_from_free_list(chunk_t * const chunk)
{
  if(chunk == free_list) {
    free_list = chunk->next;
    if(free_list != NULL) {
      free_list->prev = NULL;
    }
  } else {
    chunk->prev->next = chunk->next;
  }

  if(chunk->next != NULL) {
    chunk->next->prev = chunk->prev;
  }
}

/*
 * split_chunk: When allocating a chunk, we may have found one that is
 * larger than needed, so this function is called to keep the rest of
 * the original chunk free.
 */
static void
split_chunk(chunk_t * const chunk, size_t offset)
{
  offset = ALIGN(offset);

  if(offset + sizeof(chunk_t) < chunk->size) {
    chunk_t *new_chunk = (chunk_t *)(GET_PTR(chunk) + offset);
    new_chunk->size = chunk->size - sizeof(chunk_t) - offset;
    new_chunk->flags = 0;
    free_chunk(new_chunk);

    chunk->size = offset;
    chunk->next = chunk->prev = NULL;
  }
}

/* coalesce_chunks: Coalesce a specific free chunk with as many
   adjacent free chunks as possible. */
static void
coalesce_chunks(chunk_t *chunk)
{
  for(chunk_t *next = NEXT_CHUNK(chunk);
      (char *)next < &heap_base[heap_usage] && CHUNK_FREE(next);
      next = NEXT_CHUNK(next)) {
    chunk->size += sizeof(chunk_t) + next->size;
    LOG_DBG("Coalesce chunk of %zu bytes\n", next->size);
    remove_chunk_from_free_list(next);
  }
}

/* defrag_chunks: Scan the free list for chunks that can be coalesced,
   and stop within a bounded time. */
static void
defrag_chunks(void)
{
  /* Limit the time we spend on searching the free list. */
  int i = CHUNK_SEARCH_MAX;
  for(chunk_t *chunk = free_list; chunk != NULL; chunk = chunk->next) {
    if(i-- == 0) {
      break;
    }
    coalesce_chunks(chunk);
  }
}

/* get_free_chunk: Search the free list for the most suitable chunk,
   as determined by its size, to satisfy an allocation request. */
static chunk_t *
get_free_chunk(const size_t size)
{
  /* Defragment chunks only right before they are needed for allocation. */
  defrag_chunks();

  chunk_t *best = NULL;
  /* Limit the time we spend on searching the free list. */
  int i = CHUNK_SEARCH_MAX;
  for(chunk_t *chunk = free_list; chunk != NULL; chunk = chunk->next) {
    if(i-- == 0) {
      break;
    }

    /* To avoid fragmenting large chunks, we select the chunk with the
       smallest size that is larger than or equal to the requested size. */
    if(size <= chunk->size) {
      if(best == NULL || chunk->size < best->size) {
        best = chunk;
      }
      if(best->size == size) {
        /* We found a perfect chunk -- stop the search. */
        break;
      }
    }
  }

  if(best != NULL) {
    /* We found a chunk that can hold an object of the requested
       allocation size. Split it if possible. */
    remove_chunk_from_free_list(best);
    split_chunk(best, size);
  }

  return best;
}

/*
 * heapmem_zone_register: Register a new zone, which is essentially a
 * subdivision of the heap with a reserved allocation space. This
 * feature ensures that certain modules can get a dedicated heap for
 * prioritized memory -- unlike what can be attained when allocating
 * from the general zone.
 */
heapmem_zone_t
heapmem_zone_register(const char *name, size_t zone_size)
{
  if(zone_size > zones[HEAPMEM_ZONE_GENERAL].zone_size) {
    LOG_ERR("Too large zone allocation limit: %zu\n", zone_size);
    return HEAPMEM_ZONE_INVALID;
  } else if(name == NULL || zone_size == 0) {
    return HEAPMEM_ZONE_INVALID;
  }

  for(heapmem_zone_t i = HEAPMEM_ZONE_GENERAL + 1; i < HEAPMEM_MAX_ZONES; i++) {
    if(zones[i].name == NULL) {
      /* Found a free slot. */
      zones[i].name = name;
      zones[i].zone_size = zone_size;
      /* The general zone has a lower priority than registered zones,
         so we transfer a part of the general zone to this one. */
      zones[HEAPMEM_ZONE_GENERAL].zone_size -= zone_size;
      LOG_INFO("Registered zone \"%s\" with ID %u\n", name, i);
      return i;
    } else if(strcmp(zones[i].name, name) == 0) {
      LOG_ERR("Duplicate zone registration: %s\n", name);
      return HEAPMEM_ZONE_INVALID;
    }
  }

  LOG_ERR("Cannot allocate more zones\n");

  return HEAPMEM_ZONE_INVALID;
}

/*
 * heapmem_alloc: Allocate an object of the specified size, returning
 * a pointer to it in case of success, and NULL in case of failure.
 *
 * When allocating memory, heapmem_alloc() will first try to find a
 * free chunk of the same size as the requested one. If none can be
 * found, we pick a larger chunk that is as close in size as possible,
 * and possibly split it so that the remaining part becomes a chunk
 * available for allocation. At most CHUNK_SEARCH_MAX chunks on the
 * free list will be examined.
 *
 * As a last resort, heapmem_alloc() will try to extend the heap
 * space, and thereby create a new chunk available for use.
 */
void *
#if HEAPMEM_DEBUG
heapmem_zone_alloc_debug(heapmem_zone_t zone, size_t size,
                         const char *file, const unsigned line)
#else
heapmem_zone_alloc(heapmem_zone_t zone, size_t size)
#endif
{
  if(zone >= HEAPMEM_MAX_ZONES || zones[zone].name == NULL) {
    LOG_WARN("Attempt to allocate from invalid zone: %u\n", zone);
    return NULL;
  }

  if(size > HEAPMEM_ARENA_SIZE || size == 0) {
    return NULL;
  }

  size = ALIGN(size);

  if(sizeof(chunk_t) + size >
     zones[zone].zone_size - zones[zone].allocated) {
    LOG_ERR("Cannot allocate %zu bytes because of the zone limit\n", size);
    return NULL;
  }

  chunk_t *chunk = get_free_chunk(size);
  if(chunk == NULL) {
    chunk = extend_space(sizeof(chunk_t) + size);
    if(chunk == NULL) {
      return NULL;
    }
    chunk->size = size;
  }

  chunk->flags = CHUNK_FLAG_ALLOCATED;

#if HEAPMEM_DEBUG
  chunk->file = file;
  chunk->line = line;
#endif

  LOG_DBG("%s ptr %p size %zu\n", __func__, GET_PTR(chunk), size);

  chunk->zone = zone;
  zones[zone].allocated += sizeof(chunk_t) + size;

  return GET_PTR(chunk);
}

/*
 * heapmem_free: Deallocate a previously allocated object.
 *
 * The pointer must exactly match one returned from an earlier call
 * from heapmem_alloc or heapmem_realloc, without any call to
 * heapmem_free in between.
 *
 * When deallocating a chunk, the chunk will be inserted into the free
 * list. Moreover, all free chunks that are adjacent in memory will be
 * merged into a single chunk in order to mitigate fragmentation.
 */
bool
#if HEAPMEM_DEBUG
heapmem_free_debug(void *ptr, const char *file, const unsigned line)
#else
heapmem_free(void *ptr)
#endif
{
  if(!IN_HEAP(ptr)) {
    if(ptr) {
      LOG_WARN("%s: ptr %p is not in the heap\n", __func__, ptr);
    }
    return false;
  }

  chunk_t *chunk = GET_CHUNK(ptr);
  if(!CHUNK_ALLOCATED(chunk)) {
    LOG_WARN("%s: ptr %p has already been deallocated\n", __func__, ptr);
    return false;
  }

#if HEAPMEM_DEBUG
  LOG_DBG("%s: ptr %p, allocated at %s:%u\n", __func__, ptr,
         chunk->file, chunk->line);
#endif

  zones[chunk->zone].allocated -= sizeof(chunk_t) + chunk->size;

  free_chunk(chunk);
  return true;
}

#if HEAPMEM_REALLOC
/*
 * heapmem_realloc: Reallocate an object with a different size,
 * possibly moving it in memory. In case of success, the function
 * returns a pointer to the object's new location. In case of failure,
 * it returns NULL.
 *
 * If the size of the new chunk is larger than that of the allocated
 * chunk, heapmem_realloc() will first attempt to extend the currently
 * allocated chunk. If the adjacent memory is not free,
 * heapmem_realloc() will attempt to allocate a completely new chunk,
 * copy the old data to the new chunk, and deallocate the old chunk.
 *
 * If the size of the new chunk is smaller than the allocated one, we
 * split the allocated chunk if the remaining chunk would be large
 * enough to justify the overhead of creating a new chunk.
 */
void *
#if HEAPMEM_DEBUG
heapmem_realloc_debug(void *ptr, size_t size,
		      const char *file, const unsigned line)
#else
heapmem_realloc(void *ptr, size_t size)
#endif
{
  /* Allow the special case of ptr being NULL as an alias
     for heapmem_alloc(). */
  if(ptr != NULL && !IN_HEAP(ptr)) {
    LOG_WARN("%s: ptr %p is not in the heap\n", __func__, ptr);
    return NULL;
  }

#if HEAPMEM_DEBUG
  LOG_DBG("%s: ptr %p size %zu at %s:%u\n",
           __func__, ptr, size, file, line);
#endif

  /* Fail early on too large allocation requests to prevent wrapping values. */
  if(size > HEAPMEM_ARENA_SIZE) {
    return NULL;
  }

  /* Special cases in which we can hand off the execution to other functions. */
  if(ptr == NULL) {
    return heapmem_alloc(size);
  } else if(size == 0) {
    heapmem_free(ptr);
    return NULL;
  }

  chunk_t *chunk = GET_CHUNK(ptr);
  if(!CHUNK_ALLOCATED(chunk)) {
    LOG_WARN("%s: ptr %p is not allocated\n", __func__, ptr);
    return NULL;
  }

#if HEAPMEM_DEBUG
  chunk->file = file;
  chunk->line = line;
#endif

  size = ALIGN(size);
  int size_adj = size - chunk->size;

  if(size_adj <= 0) {
    /* Request to make the object smaller or to keep its size.
       In the former case, the chunk will be split if possible. */
    split_chunk(chunk, size);
    zones[chunk->zone].allocated += size_adj;
    return ptr;
  }

  /* Request to make the object larger. (size_adj > 0) */
  if(IS_LAST_CHUNK(chunk)) {
    /*
     * If the object belongs to the last allocated chunk (i.e., the
     * one before the end of the heap footprint, we just attempt to
     * extend the heap.
     */
    if(extend_space(size_adj) != NULL) {
      chunk->size = size;
      zones[chunk->zone].allocated += size_adj;
      return ptr;
    }
  } else {
    /*
     * Here we attempt to enlarge an allocated object, whose
     * adjacent space may already be allocated. We attempt to
     * coalesce chunks in order to make as much room as possible.
     */
    coalesce_chunks(chunk);
    if(chunk->size >= size) {
      /* There was enough free adjacent space to extend the chunk in
	 its current place. */
      split_chunk(chunk, size);
      zones[chunk->zone].allocated += size_adj;
      return ptr;
    }
  }

  /*
   * Failed to enlarge the object in its current place, since the
   * adjacent chunk is allocated. Hence, we try to place the new
   * object elsewhere in the heap, and remove the old chunk that was
   * holding it.
   */
  void *newptr = heapmem_zone_alloc(chunk->zone, size);
  if(newptr == NULL) {
    return NULL;
  }

  memcpy(newptr, ptr, chunk->size);
  free_chunk(chunk);

  return newptr;
}
#endif /* HEAPMEM_REALLOC */

/* heapmem_calloc: Allocates memory for a zero-initialized array. */
void *
#if HEAPMEM_DEBUG
heapmem_calloc_debug(size_t nmemb, size_t size,
		     const char *file, const unsigned line)
#else
heapmem_calloc(size_t nmemb, size_t size)
#endif
{
  size_t total_size = nmemb * size;

  /* Overflow check. */
  if(size == 0 || total_size / size != nmemb) {
    return NULL;
  }

  void *ptr = heapmem_alloc(total_size);
  if(ptr != NULL) {
    memset(ptr, 0, total_size);
  }
  return ptr;
}

/* heapmem_stats: Provides statistics regarding heap memory usage. */
void
heapmem_stats(heapmem_stats_t *stats)
{
  memset(stats, 0, sizeof(*stats));

  for(chunk_t *chunk = (chunk_t *)heap_base;
      (char *)chunk < &heap_base[heap_usage];
      chunk = NEXT_CHUNK(chunk)) {
    if(CHUNK_ALLOCATED(chunk)) {
      stats->allocated += chunk->size;
      stats->overhead += sizeof(chunk_t);
    } else {
      coalesce_chunks(chunk);
      stats->available += chunk->size;
    }
  }
  stats->available += HEAPMEM_ARENA_SIZE - heap_usage;
  stats->footprint = heap_usage;
  stats->max_footprint = max_heap_usage;
  stats->chunks = stats->overhead / sizeof(chunk_t);
}

/* heapmem_print_stats: Print all the statistics collected through the
   heapmem_stats function. */
void
heapmem_print_debug_info(bool print_chunks)
{
  heapmem_stats_t stats;
  heapmem_stats(&stats);

  HEAPMEM_PRINTF("* HeapMem statistics\n");
  HEAPMEM_PRINTF("* Allocated memory: %zu\n", stats.allocated);
  HEAPMEM_PRINTF("* Available memory: %zu\n", stats.available);
  HEAPMEM_PRINTF("* Heap usage: %zu\n", stats.footprint);
  HEAPMEM_PRINTF("* Max heap usage: %zu\n", stats.max_footprint);
  HEAPMEM_PRINTF("* Allocated chunks: %zu\n", stats.chunks);
  HEAPMEM_PRINTF("* Chunk size: %zu\n", sizeof(chunk_t));
  HEAPMEM_PRINTF("* Total chunk overhead: %zu\n", stats.overhead);

  if(print_chunks) {
    HEAPMEM_PRINTF("* Allocated chunks:\n");
    for(chunk_t *chunk = (chunk_t *)heap_base;
        (char *)chunk < &heap_base[heap_usage];
        chunk = NEXT_CHUNK(chunk)) {
      if(CHUNK_ALLOCATED(chunk)) {
#if HEAPMEM_DEBUG
        HEAPMEM_PRINTF("* Chunk: heap offset %"PRIuPTR", obj %p, flags 0x%x (%s:%u)\n",
                       (uintptr_t)((char *)chunk - (char *)heap_base),
                       GET_PTR(chunk), chunk->flags, chunk->file, chunk->line);
#else
        HEAPMEM_PRINTF("* Chunk: heap offset %"PRIuPTR", obj %p, flags 0x%x\n",
                       (uintptr_t)((char *)chunk - (char *)heap_base),
                       GET_PTR(chunk), chunk->flags);
#endif /* HEAPMEM_DEBUG */
      }
    }
  }
}

/* heapmem_alignment: Returns the minimum alignment of allocated addresses. */
size_t
heapmem_alignment(void)
{
  return HEAPMEM_ALIGNMENT;
}

#endif /* HEAPMEM_CONF_ARENA_SIZE */

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

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "contiki.h"
#include "lib/heapmem.h"
#include "sys/cc.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "HeapMem"
#define LOG_LEVEL LOG_LEVEL_WARN

/* The HEAPMEM_CONF_PRINTF parameter determines which function to use for
   printing debug information. It defaults to LOG_OUTPUT. */
#ifdef HEAPMEM_CONF_PRINTF
#define HEAPMEM_PRINTF(...) HEAPMEM_CONF_PRINTF(__VA_ARGS__)
#else
#define HEAPMEM_PRINTF(...) LOG_OUTPUT(__VA_ARGS__)
#endif /* HEAPMEM_CONF_PRINTF */

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

_Static_assert((HEAPMEM_ALIGNMENT & (HEAPMEM_ALIGNMENT - 1)) == 0,
               "HEAPMEM_ALIGNMENT must be a power of 2");

static inline size_t
align_size(size_t size)
{
  if(size > SIZE_MAX - (HEAPMEM_ALIGNMENT - 1)) {
    return 0;
  }
  return ((size + (HEAPMEM_ALIGNMENT - 1)) & ~(HEAPMEM_ALIGNMENT - 1));
}

#define ALIGN(size) align_size(size)

/* Macros for chunk iteration. */
#define NEXT_CHUNK(chunk)						\
  ((chunk_t *)((char *)(chunk) + sizeof(chunk_t) + (chunk)->size))
#define IS_LAST_CHUNK(zone, chunk)					\
  ((char *)NEXT_CHUNK(chunk) == (zone)->heap_base + (zone)->heap_usage)

/* Macros for retrieving the data pointer from a chunk, and the other
   way around. */
#define GET_CHUNK(ptr)				\
  ((chunk_t *)((char *)(ptr) - sizeof(chunk_t)))
#define GET_PTR(chunk)				\
  (char *)((chunk) + 1)

/*
 * We use a double-linked list of chunks, with a slight space overhead
 * compared to a single-linked list, but with the advantage of having
 * much faster list removals.
 */
typedef struct heapmem_chunk {
  struct heapmem_chunk *prev;
  struct heapmem_chunk *next;
  size_t size;
  bool allocated;
#if HEAPMEM_DEBUG
  const char *file;
  unsigned line;
#endif
} chunk_t;

_Static_assert(sizeof(chunk_t) % HEAPMEM_ALIGNMENT == 0,
               "sizeof(chunk_t) must be a multiple of HEAPMEM_ALIGNMENT");

/* The general zone with a statically allocated arena. */
#ifdef HEAPMEM_CONF_ARENA_SIZE
#define HEAPMEM_ARENA_SIZE HEAPMEM_CONF_ARENA_SIZE
static char heapmem_general_buf_[HEAPMEM_ARENA_SIZE] CC_ALIGN(HEAPMEM_ALIGNMENT);
static heapmem_zone_t heapmem_zone_general = {
  .name = "GENERAL",
  .heap_base = heapmem_general_buf_,
  .arena_size = HEAPMEM_ARENA_SIZE,
};
#endif /* HEAPMEM_CONF_ARENA_SIZE */

/*
 * resolve_zone: Map a NULL zone pointer to the general zone (when
 * available). This allows convenience macros to pass NULL instead of
 * exposing the general zone as a global variable.
 */
static inline heapmem_zone_t *
resolve_zone(heapmem_zone_t *zone)
{
#ifdef HEAPMEM_CONF_ARENA_SIZE
  if(zone == NULL) {
    return &heapmem_zone_general;
  }
#endif
  return zone;
}

#define IN_ZONE(zone, ptr) ((ptr) != NULL &&				\
			    (char *)(ptr) >= (zone)->heap_base + sizeof(chunk_t) && \
			    (char *)(ptr) < (zone)->heap_base + (zone)->heap_usage)

/* extend_space: Increases the current footprint used in the zone's heap, and
   returns a pointer to the old end. */
static void *
extend_space(heapmem_zone_t *zone, size_t size)
{
  if(size > zone->arena_size - zone->heap_usage) {
    return NULL;
  }

  char *old_usage = zone->heap_base + zone->heap_usage;
  zone->heap_usage += size;
  if(zone->heap_usage > zone->max_heap_usage) {
    zone->max_heap_usage = zone->heap_usage;
  }

  return old_usage;
}

/* free_chunk: Mark a chunk as being free, and put it on the free list. */
static void
free_chunk(heapmem_zone_t *zone, chunk_t * const chunk)
{
  chunk->allocated = false;

  if(IS_LAST_CHUNK(zone, chunk)) {
    /* Release the chunk back into the wilderness. */
    zone->heap_usage -= sizeof(chunk_t) + chunk->size;
  } else {
    /* Put the chunk on the free list. */
    chunk->prev = NULL;
    chunk->next = zone->free_list;
    if(zone->free_list != NULL) {
      zone->free_list->prev = chunk;
    }
    zone->free_list = chunk;
  }
}

/* remove_chunk_from_free_list: Mark a chunk as being allocated, and
   remove it from the free list. */
static void
remove_chunk_from_free_list(heapmem_zone_t *zone, chunk_t * const chunk)
{
  if(chunk == zone->free_list) {
    zone->free_list = chunk->next;
    if(zone->free_list != NULL) {
      zone->free_list->prev = NULL;
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
split_chunk(heapmem_zone_t *zone, chunk_t * const chunk, size_t offset)
{
  offset = ALIGN(offset);

  if(offset + sizeof(chunk_t) < chunk->size) {
    chunk_t *new_chunk = (chunk_t *)(GET_PTR(chunk) + offset);
    new_chunk->size = chunk->size - sizeof(chunk_t) - offset;
    new_chunk->allocated = false;
    free_chunk(zone, new_chunk);

    chunk->size = offset;
    chunk->next = chunk->prev = NULL;
  }
}

/* coalesce_chunks: Coalesce a specific free chunk with as many
   adjacent free chunks as possible. */
static void
coalesce_chunks(heapmem_zone_t *zone, chunk_t *chunk)
{
  for(chunk_t *next = NEXT_CHUNK(chunk);
      (char *)next < zone->heap_base + zone->heap_usage && !next->allocated;
      next = NEXT_CHUNK(next)) {
    chunk->size += sizeof(chunk_t) + next->size;
    LOG_DBG("Coalesce chunk of %zu bytes\n", next->size);
    remove_chunk_from_free_list(zone, next);
  }
}

/* defrag_chunks: Scan the free list for chunks that can be coalesced,
   and stop within a bounded time. */
static void
defrag_chunks(heapmem_zone_t *zone)
{
  /* Limit the time we spend on searching the free list. */
  int i = CHUNK_SEARCH_MAX;
  for(chunk_t *chunk = zone->free_list; chunk != NULL; chunk = chunk->next) {
    if(i-- == 0) {
      break;
    }
    coalesce_chunks(zone, chunk);
  }
}

/* get_free_chunk: Search the free list for the most suitable chunk,
   as determined by its size, to satisfy an allocation request. */
static chunk_t *
get_free_chunk(heapmem_zone_t *zone, const size_t size)
{
  /* Defragment chunks only right before they are needed for allocation. */
  defrag_chunks(zone);

  chunk_t *best = NULL;
  /* Limit the time we spend on searching the free list. */
  int i = CHUNK_SEARCH_MAX;
  for(chunk_t *chunk = zone->free_list; chunk != NULL; chunk = chunk->next) {
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
    remove_chunk_from_free_list(zone, best);
    split_chunk(zone, best, size);
  }

  return best;
}

/*
 * heapmem_zone_alloc: Allocate an object of the specified size from the
 * given zone, returning a pointer to it in case of success, and NULL
 * in case of failure.
 *
 * When allocating memory, heapmem_zone_alloc() will first try to find a
 * free chunk of the same size as the requested one. If none can be
 * found, we pick a larger chunk that is as close in size as possible,
 * and possibly split it so that the remaining part becomes a chunk
 * available for allocation. At most CHUNK_SEARCH_MAX chunks on the
 * free list will be examined.
 *
 * As a last resort, heapmem_zone_alloc() will try to extend the heap
 * space, and thereby create a new chunk available for use.
 */
static void *
zone_alloc(heapmem_zone_t *zone, size_t size,
           const char *file, const unsigned line)
{
  zone = resolve_zone(zone);
  if(zone == NULL || zone->heap_base == NULL) {
    LOG_WARN("Attempt to allocate from invalid zone\n");
    return NULL;
  }

  if(size > zone->arena_size || size == 0) {
    return NULL;
  }

  size = ALIGN(size);
  if(size == 0) {
    LOG_ERR("Size overflow in alignment\n");
    return NULL;
  }

  chunk_t *chunk = get_free_chunk(zone, size);
  if(chunk == NULL) {
    chunk = extend_space(zone, sizeof(chunk_t) + size);
    if(chunk == NULL) {
      return NULL;
    }
    chunk->size = size;
  }

  chunk->allocated = true;

#if HEAPMEM_DEBUG
  chunk->file = file;
  chunk->line = line;
#endif

  LOG_DBG("zone_alloc: zone \"%s\" ptr %p size %zu\n",
          zone->name, GET_PTR(chunk), chunk->size);

  return GET_PTR(chunk);
}

#if HEAPMEM_DEBUG
void *
heapmem_zone_alloc_debug(heapmem_zone_t *zone, size_t size,
                          const char *file, const unsigned line)
{
  return zone_alloc(zone, size, file, line);
}
#else
void *
heapmem_zone_alloc(heapmem_zone_t *zone, size_t size)
{
  return zone_alloc(zone, size, NULL, 0);
}
#endif

/*
 * heapmem_zone_free: Deallocate a previously allocated object.
 *
 * The pointer must exactly match one returned from an earlier call
 * from heapmem_zone_alloc or heapmem_zone_realloc, without any call to
 * heapmem_zone_free in between.
 *
 * When deallocating a chunk, the chunk will be inserted into the free
 * list. Moreover, all free chunks that are adjacent in memory will be
 * merged into a single chunk in order to mitigate fragmentation.
 */
static bool
zone_free(heapmem_zone_t *zone, void *ptr,
          const char *file, const unsigned line)
{
  zone = resolve_zone(zone);
  if(zone == NULL || !IN_ZONE(zone, ptr)) {
    if(ptr) {
      LOG_WARN("zone_free: ptr %p is not in the zone\n", ptr);
    }
    return false;
  }

  chunk_t *chunk = GET_CHUNK(ptr);
  if(!chunk->allocated) {
    LOG_WARN("zone_free: ptr %p has already been deallocated\n", ptr);
    return false;
  }

#if HEAPMEM_DEBUG
  LOG_DBG("zone_free: ptr %p, allocated at %s:%u\n", ptr,
         chunk->file, chunk->line);
#endif

  free_chunk(zone, chunk);
  return true;
}

#if HEAPMEM_DEBUG
bool
heapmem_zone_free_debug(heapmem_zone_t *zone, void *ptr,
                         const char *file, const unsigned line)
{
  return zone_free(zone, ptr, file, line);
}
#else
bool
heapmem_zone_free(heapmem_zone_t *zone, void *ptr)
{
  return zone_free(zone, ptr, NULL, 0);
}
#endif

/*
 * heapmem_zone_realloc: Reallocate an object with a different size,
 * possibly moving it in memory. In case of success, the function
 * returns a pointer to the object's new location. In case of failure,
 * it returns NULL.
 *
 * If the size of the new chunk is larger than that of the allocated
 * chunk, heapmem_zone_realloc() will first attempt to extend the
 * currently allocated chunk. If the adjacent memory is not free,
 * heapmem_zone_realloc() will attempt to allocate a completely new
 * chunk, copy the old data to the new chunk, and deallocate the old
 * chunk.
 *
 * If the size of the new chunk is smaller than the allocated one, we
 * split the allocated chunk if the remaining chunk would be large
 * enough to justify the overhead of creating a new chunk.
 */
static void *
zone_realloc(heapmem_zone_t *zone, void *ptr, size_t size,
             const char *file, const unsigned line)
{
  zone = resolve_zone(zone);

  /* Allow the special case of ptr being NULL as an alias
     for heapmem_zone_alloc(). */
  if(zone == NULL) {
    LOG_WARN("Attempt to use invalid zone\n");
    return NULL;
  }
  if(ptr != NULL && !IN_ZONE(zone, ptr)) {
    LOG_WARN("zone_realloc: ptr %p is not in the zone\n", ptr);
    return NULL;
  }

#if HEAPMEM_DEBUG
  LOG_DBG("zone_realloc: ptr %p size %zu at %s:%u\n",
           ptr, size, file, line);
#endif

  /* Fail early on too large allocation requests to prevent wrapping values. */
  if(size > zone->arena_size) {
    return NULL;
  }

  /* Special cases in which we can hand off the execution to other functions. */
  if(ptr == NULL) {
    return zone_alloc(zone, size, file, line);
  } else if(size == 0) {
    zone_free(zone, ptr, file, line);
    return NULL;
  }

  chunk_t *chunk = GET_CHUNK(ptr);
  if(!chunk->allocated) {
    LOG_WARN("zone_realloc: ptr %p is not allocated\n", ptr);
    return NULL;
  }

#if HEAPMEM_DEBUG
  chunk->file = file;
  chunk->line = line;
#endif

  size = ALIGN(size);
  if(size == 0) {
    LOG_ERR("Size overflow in alignment\n");
    return NULL;
  }

  size_t old_size = chunk->size;

  if(size <= old_size) {
    /* Request to make the object smaller or to keep its size.
       In the former case, the chunk will be split if possible. */
    split_chunk(zone, chunk, size);
    return ptr;
  }

  /* Request to make the object larger. */
  size_t size_increase = size - old_size;

  if(IS_LAST_CHUNK(zone, chunk)) {
    /*
     * If the object belongs to the last allocated chunk (i.e., the
     * one before the end of the heap footprint, we just attempt to
     * extend the heap.
     */
    if(extend_space(zone, size_increase) != NULL) {
      chunk->size = size;
      return ptr;
    }
  } else {
    /*
     * Here we attempt to enlarge an allocated object, whose
     * adjacent space may already be allocated. We attempt to
     * coalesce chunks in order to make as much room as possible.
     */
    coalesce_chunks(zone, chunk);
    if(chunk->size >= size) {
      /* There was enough free adjacent space to extend the chunk in
	 its current place. */
      split_chunk(zone, chunk, size);
      return ptr;
    }
  }

  /*
   * Failed to enlarge the object in its current place, since the
   * adjacent chunk is allocated. Hence, we try to place the new
   * object elsewhere in the heap, and remove the old chunk that was
   * holding it.
   */
  void *newptr = zone_alloc(zone, size, file, line);
  if(newptr == NULL) {
    return NULL;
  }

  memcpy(newptr, ptr, old_size);
  zone_free(zone, ptr, file, line);

  return newptr;
}

#if HEAPMEM_DEBUG
void *
heapmem_zone_realloc_debug(heapmem_zone_t *zone, void *ptr, size_t size,
                            const char *file, const unsigned line)
{
  return zone_realloc(zone, ptr, size, file, line);
}
#else
void *
heapmem_zone_realloc(heapmem_zone_t *zone, void *ptr, size_t size)
{
  return zone_realloc(zone, ptr, size, NULL, 0);
}
#endif

static void *
zone_calloc(heapmem_zone_t *zone, size_t nmemb, size_t size,
            const char *file, const unsigned line)
{
  size_t total_size = nmemb * size;

  /* Overflow check. */
  if(size == 0 || total_size / size != nmemb) {
    return NULL;
  }

  void *ptr = zone_alloc(zone, total_size, file, line);
  if(ptr != NULL) {
    memset(ptr, 0, total_size);
  }
  return ptr;
}

#if HEAPMEM_DEBUG
void *
heapmem_zone_calloc_debug(heapmem_zone_t *zone, size_t nmemb, size_t size,
                           const char *file, const unsigned line)
{
  return zone_calloc(zone, nmemb, size, file, line);
}
#else
void *
heapmem_zone_calloc(heapmem_zone_t *zone, size_t nmemb, size_t size)
{
  return zone_calloc(zone, nmemb, size, NULL, 0);
}
#endif

/* heapmem_zone_stats: Provides statistics regarding zone memory usage. */
void
heapmem_zone_stats(heapmem_zone_t *zone, heapmem_stats_t *stats)
{
  zone = resolve_zone(zone);
  memset(stats, 0, sizeof(*stats));
  if(zone == NULL) {
    return;
  }

  for(chunk_t *chunk = (chunk_t *)zone->heap_base;
      (char *)chunk < zone->heap_base + zone->heap_usage;
      chunk = NEXT_CHUNK(chunk)) {
    stats->overhead += sizeof(chunk_t);
    if(chunk->allocated) {
      stats->allocated += chunk->size;
      stats->chunks++;
    } else {
      coalesce_chunks(zone, chunk);
      stats->available += chunk->size;
    }
  }
  stats->available += zone->arena_size - zone->heap_usage;
  stats->heap_usage = zone->heap_usage;
  stats->max_heap_usage = zone->max_heap_usage;
}

/* heapmem_zone_print_debug_info: Print statistics and optionally
   chunk details for a specific zone. */
void
heapmem_zone_print_debug_info(heapmem_zone_t *zone, bool print_chunks)
{
  zone = resolve_zone(zone);
  if(zone == NULL) {
    return;
  }

  heapmem_stats_t stats;
  heapmem_zone_stats(zone, &stats);

  HEAPMEM_PRINTF("* HeapMem zone \"%s\" statistics\n", zone->name);
  HEAPMEM_PRINTF("* Arena size: %zu\n", zone->arena_size);
  HEAPMEM_PRINTF("* Allocated memory: %zu\n", stats.allocated);
  HEAPMEM_PRINTF("* Available memory: %zu\n", stats.available);
  HEAPMEM_PRINTF("* Heap usage: %zu\n", stats.heap_usage);
  HEAPMEM_PRINTF("* Max heap usage: %zu\n", stats.max_heap_usage);
  HEAPMEM_PRINTF("* Allocated chunks: %zu\n", stats.chunks);
  HEAPMEM_PRINTF("* Chunk size: %zu\n", sizeof(chunk_t));
  HEAPMEM_PRINTF("* Total chunk overhead: %zu\n", stats.overhead);

  if(print_chunks) {
    HEAPMEM_PRINTF("* Allocated chunks:\n");
    for(chunk_t *chunk = (chunk_t *)zone->heap_base;
        (char *)chunk < zone->heap_base + zone->heap_usage;
        chunk = NEXT_CHUNK(chunk)) {
      if(chunk->allocated) {
#if HEAPMEM_DEBUG
        HEAPMEM_PRINTF("* Chunk: heap offset %"PRIuPTR", obj %p, size %zu (%s:%u)\n",
                       (uintptr_t)((char *)chunk - zone->heap_base),
                       GET_PTR(chunk), chunk->size, chunk->file, chunk->line);
#else
        HEAPMEM_PRINTF("* Chunk: heap offset %"PRIuPTR", obj %p, size %zu\n",
                       (uintptr_t)((char *)chunk - zone->heap_base),
                       GET_PTR(chunk), chunk->size);
#endif /* HEAPMEM_DEBUG */
      }
    }
  }
}


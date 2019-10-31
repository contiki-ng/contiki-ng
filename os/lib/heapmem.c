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
 * 	Dynamic memory allocation module.
 * \author
 * 	Nicolas Tsiftes <nvt@acm.org>
 */

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#undef HEAPMEM_DEBUG
#define HEAPMEM_DEBUG 1
#else
#define PRINTF(...)
#endif

#ifdef PROJECT_CONF_PATH
/* Load the heapmem configuration from a project configuration file. */
#include PROJECT_CONF_PATH
#endif

#include <stdint.h>
#include <string.h>

#include "heapmem.h"

#include "sys/cc.h"

/* The HEAPMEM_CONF_ARENA_SIZE parameter determines the size of the
   space that will be statically allocated in this module. */
#ifdef HEAPMEM_CONF_ARENA_SIZE
#define HEAPMEM_ARENA_SIZE HEAPMEM_CONF_ARENA_SIZE
#else
/* If the heap size is not set, we use a minimal size that will ensure
   that all allocation attempts fail. */
#define HEAPMEM_ARENA_SIZE 1
#endif
/* HEAPMEM_CONF_ARENA_SIZE */

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
 * The HEAPMEM_CONF_REALLOC parameter determines whether heapmem_realloc() is
 * enabled (non-zero value) or not (zero value).
 */
#ifdef HEAPMEM_CONF_REALLOC
#define HEAPMEM_REALLOC HEAPMEM_CONF_REALLOC
#else
#define HEAPMEM_REALLOC 1
#endif /* HEAPMEM_CONF_REALLOC */

/*
 * The HEAPMEM_CONF_ALIGNMENT parameter decides what the minimum alignment
 * for allocated data should be.
 */
#ifdef HEAPMEM_CONF_ALIGNMENT
#define HEAPMEM_ALIGNMENT HEAPMEM_CONF_ALIGNMENT
#else
#define HEAPMEM_ALIGNMENT sizeof(int)
#endif /* HEAPMEM_CONF_ALIGNMENT */

#define ALIGN(size)						\
  (((size) + (HEAPMEM_ALIGNMENT - 1)) & ~(HEAPMEM_ALIGNMENT - 1))

/* Macros for chunk iteration. */
#define NEXT_CHUNK(chunk)						\
  ((chunk_t *)((char *)(chunk) + sizeof(chunk_t) + (chunk)->size))
#define IS_LAST_CHUNK(chunk)			\
  ((char *)NEXT_CHUNK(chunk) == &heap_base[heap_usage])

/* Macros for retrieving the data pointer from a chunk,
   and the other way around. */
#define GET_CHUNK(ptr)				\
  ((chunk_t *)((char *)(ptr) - sizeof(chunk_t)))
#define GET_PTR(chunk)				\
  (char *)((chunk) + 1)

/* Macros for determining the status of a chunk. */
#define CHUNK_FLAG_ALLOCATED		0x1

#define CHUNK_ALLOCATED(chunk)			\
  ((chunk)->flags & CHUNK_FLAG_ALLOCATED)
#define CHUNK_FREE(chunk)			\
  (~(chunk)->flags & CHUNK_FLAG_ALLOCATED)

/*
 * We use a double-linked list of chunks, with a slight space overhead compared
 * to a single-linked list, but with the advantage of having much faster
 * list removals.
 */
typedef struct chunk {
  struct chunk *prev;
  struct chunk *next;
  size_t size;
  uint8_t flags;
#if HEAPMEM_DEBUG
  const char *file;
  unsigned line;
#endif
} chunk_t;

/* All allocated space is located within an "heap", which is statically
   allocated with a pre-configured size. */
static char heap_base[HEAPMEM_ARENA_SIZE] CC_ALIGN(HEAPMEM_ALIGNMENT);
static size_t heap_usage;

static chunk_t *first_chunk = (chunk_t *)heap_base;
static chunk_t *free_list;

/* extend_space: Increases the current footprint used in the heap, and
   returns a pointer to the old end. */
static void *
extend_space(size_t size)
{
  char *old_usage;

  if(heap_usage + size > HEAPMEM_ARENA_SIZE) {
    return NULL;
  }

  old_usage = &heap_base[heap_usage];
  heap_usage += size;

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

/* allocate_chunk: Mark a chunk as being allocated, and remove it
   from the free list. */
static void
allocate_chunk(chunk_t * const chunk)
{
  chunk->flags |= CHUNK_FLAG_ALLOCATED;

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
  chunk_t *new_chunk;

  offset = ALIGN(offset);

  if(offset + sizeof(chunk_t) < chunk->size) {
    new_chunk = (chunk_t *)(GET_PTR(chunk) + offset);
    new_chunk->size = chunk->size - sizeof(chunk_t) - offset;
    new_chunk->flags = 0;
    free_chunk(new_chunk);

    chunk->size = offset;
    chunk->next = chunk->prev = NULL;
  }
}

/* coalesce_chunks: Coalesce a specific free chunk with as many adjacent
   free chunks as possible. */
static void
coalesce_chunks(chunk_t *chunk)
{
  chunk_t *next;

  for(next = NEXT_CHUNK(chunk);
      (char *)next < &heap_base[heap_usage] && CHUNK_FREE(next);
      next = NEXT_CHUNK(next)) {
    chunk->size += sizeof(chunk_t) + next->size;
    allocate_chunk(next);
  }
}

/* defrag_chunks: Scan the free list for chunks that can be coalesced,
   and stop within a bounded time. */
static void
defrag_chunks(void)
{
  int i;
  chunk_t *chunk;

  /* Limit the time we spend on searching the free list. */
  i = CHUNK_SEARCH_MAX;
  for(chunk = free_list; chunk != NULL; chunk = chunk->next) {
    if(i-- == 0) {
      break;
    }
    coalesce_chunks(chunk);
  }
}

/* get_free_chunk: Search the free list for the most suitable chunk, as
   determined by its size, to satisfy an allocation request. */
static chunk_t *
get_free_chunk(const size_t size)
{
  int i;
  chunk_t *chunk, *best;

  /* Defragment chunks only right before they are needed for allocation. */
  defrag_chunks();

  best = NULL;
  /* Limit the time we spend on searching the free list. */
  i = CHUNK_SEARCH_MAX;
  for(chunk = free_list; chunk != NULL; chunk = chunk->next) {
    if(i-- == 0) {
      break;
    }

    /*
     * To avoid fragmenting large chunks, we select the chunk with the
     * smallest size that is larger than or equal to the requested size.
     */
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
    /* We found a chunk for the allocation. Split it if necessary. */
    allocate_chunk(best);
    split_chunk(best, size);
  }

  return best;
}

/*
 * heapmem_alloc: Allocate an object of the specified size, returning
 * a pointer to it in case of success, and NULL in case of failure.
 *
 * When allocating memory, heapmem_alloc() will first try to find a
 * free chunk of the same size and the requested one. If none can be
 * find, we pick a larger chunk that is as close in size as possible,
 * and possibly split it so that the remaining part becomes a chunk
 * available for allocation.  At most CHUNK_SEARCH_MAX chunks on the
 * free list will be examined.
 *
 * As a last resort, heapmem_alloc() will try to extend the heap
 * space, and thereby create a new chunk available for use.
 */
void *
#if HEAPMEM_DEBUG
heapmem_alloc_debug(size_t size, const char *file, const unsigned line)
#else
heapmem_alloc(size_t size)
#endif
{
  chunk_t *chunk;

  size = ALIGN(size);

  chunk = get_free_chunk(size);
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

  PRINTF("%s ptr %p size %lu\n", __func__, GET_PTR(chunk), (unsigned long)size);

  return GET_PTR(chunk);
}

/*
 * heapmem_free: Deallocate a previously allocated object.
 *
 * The pointer must exactly match one returned from an earlier call
 * from heapmem_alloc or heapmem_realloc, without any call to
 * heapmem_free in between.
 *
 * When performing a deallocation of a chunk, the chunk will be put on
 * a list of free chunks internally. All free chunks that are adjacent
 * in memory will be merged into a single chunk in order to mitigate
 * fragmentation.
 */
void
#if HEAPMEM_DEBUG
heapmem_free_debug(void *ptr, const char *file, const unsigned line)
#else
heapmem_free(void *ptr)
#endif
{
  chunk_t *chunk;

  if(ptr) {
    chunk = GET_CHUNK(ptr);

    PRINTF("%s ptr %p, allocated at %s:%u\n", __func__, ptr,
           chunk->file, chunk->line);

    free_chunk(chunk);
  }
}

#if HEAPMEM_REALLOC
/*
 * heapmem_realloc: Reallocate an object with a different size,
 * possibly moving it in memory. In case of success, the function
 * returns a pointer to the objects new location. In case of failure,
 * it returns NULL.
 *
 * If the size of the new chunk is larger than that of the allocated
 * chunk, heapmem_realloc() will first attempt to extend the currently
 * allocated chunk. If that memory is not free, heapmem_ralloc() will
 * attempt to allocate a completely new chunk, copy the old data to
 * the new chunk, and deallocate the old chunk.
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
  void *newptr;
  chunk_t *chunk;
  int size_adj;

  PRINTF("%s ptr %p size %u at %s:%u\n",
         __func__, ptr, (unsigned)size, file, line);

  /* Special cases in which we can hand off the execution to other functions. */
  if(ptr == NULL) {
    return heapmem_alloc(size);
  } else if(size == 0) {
    heapmem_free(ptr);
    return NULL;
  }

  chunk = GET_CHUNK(ptr);
#if HEAPMEM_DEBUG
  chunk->file = file;
  chunk->line = line;
#endif

  size = ALIGN(size);
  size_adj = size - chunk->size;

  if(size_adj <= 0) {
    /* Request to make the object smaller or to keep its size.
       In the former case, the chunk will be split if possible. */
    split_chunk(chunk, size);
    return ptr;
  }

  /* Request to make the object larger. (size_adj > 0) */
  if(IS_LAST_CHUNK(chunk)) {
    /*
     * If the object is within the last allocated chunk (i.e., the
     * one before the end of the heap footprint, we just attempt to
     * extend the heap.
     */
    if(extend_space(size_adj) != NULL) {
      chunk->size = size;
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
      return ptr;
    }
  }

  /*
   * Failed to enlarge the object in its current place, since the
   * adjacent chunk is allocated. Hence, we try to place the new
   * object elsewhere in the heap, and remove the old chunk that was
   * holding it.
   */
  newptr = heapmem_alloc(size);
  if(newptr == NULL) {
    return NULL;
  }

  memcpy(newptr, ptr, chunk->size);
  free_chunk(chunk);

  return newptr;
}
#endif /* HEAPMEM_REALLOC */

/* heapmem_stats: Calculate statistics regarding memory usage. */
void
heapmem_stats(heapmem_stats_t *stats)
{
  chunk_t *chunk;

  memset(stats, 0, sizeof(*stats));

  for(chunk = first_chunk;
      (char *)chunk < &heap_base[heap_usage];
      chunk = NEXT_CHUNK(chunk)) {
    if(CHUNK_ALLOCATED(chunk)) {
      stats->allocated += chunk->size;
    } else {
      coalesce_chunks(chunk);
      stats->available += chunk->size;
    }
    stats->overhead += sizeof(chunk_t);
  }
  stats->available += HEAPMEM_ARENA_SIZE - heap_usage;
  stats->footprint = heap_usage;
  stats->chunks = stats->overhead / sizeof(chunk_t);
}

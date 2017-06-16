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
 * \file
 * 	Declarations for the dynamic memory allocation module.
 * \author
 * 	Nicolas Tsiftes <nvt@sics.se>
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

void *heapmem_alloc_debug(size_t, const char *file, const unsigned line);
void *heapmem_realloc_debug(void *, size_t, const char *file, const unsigned line);
void heapmem_free_debug(void *, const char *file, const unsigned line);

#else

void *heapmem_alloc(size_t);
void *heapmem_realloc(void *, size_t);
void heapmem_free(void *);
#endif /* HEAMMEM_DEBUG */

void heapmem_stats(heapmem_stats_t *);

#endif /* !HEAPMEM_H */

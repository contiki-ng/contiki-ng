/*
 * Copyright (c) 2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the Contiki desktop OS
 *
 *
 */

/**
 * \file
 * Default definitions of C compiler quirk work-arounds.
 * \author Adam Dunkels <adam@dunkels.com>
 *
 * This file is used for making use of extra functionality of some C
 * compilers used for Contiki, and defining work-arounds for various
 * quirks and problems with some other C compilers.
 */

#ifndef CC_H_
#define CC_H_

#include "contiki.h"

#ifdef __GNUC__

#define CC_CONF_ALIGN(n) __attribute__((__aligned__(n)))

#ifndef CC_CONF_CONSTRUCTOR
#define CC_CONF_CONSTRUCTOR(prio) __attribute__((constructor(prio)))
#endif /* CC_CONF_CONSTRUCTOR */

#ifndef CC_CONF_DESTRUCTOR
#define CC_CONF_DESTRUCTOR(prio) __attribute__((destructor(prio)))
#endif /* CC_CONF_DESTRUCTOR */

#define CC_CONF_DEPRECATED(msg) __attribute__((deprecated(msg)))

#define CC_CONF_NORETURN __attribute__((__noreturn__))

#endif /* __GNUC__ */

#ifdef CC_CONF_ALIGN
#define CC_ALIGN(n) CC_CONF_ALIGN(n)
#endif /* CC_CONF_ALIGN */

/**
 * Configure if the C compiler supports functions that are not meant to return
 * e.g. with __attribute__((__noreturn__))
 */
#ifdef CC_CONF_NORETURN
#define CC_NORETURN CC_CONF_NORETURN
#else
#define CC_NORETURN
#endif /* CC_CONF_NORETURN */

/**
 * Configure if the C compiler supports marking functions as constructors
 * e.g. with __attribute__((constructor(prio))).
 *
 * Lower priority runs before higher priority. Priorities 0-100 are reserved.
 */
#ifdef CC_CONF_CONSTRUCTOR
#define CC_CONSTRUCTOR(prio) CC_CONF_CONSTRUCTOR(prio)
#else
#define CC_CONSTRUCTOR(prio)
#endif /* CC_CONF_CONSTRUCTOR */

/**
 * Configure if the C compiler supports marking functions as destructors
 * e.g. with __attribute__((destructor(prio))).
 *
 * Lower priority runs before higher priority. Priorities 0-100 are reserved.
 */
#ifdef CC_CONF_DESTRUCTOR
#define CC_DESTRUCTOR(prio) CC_CONF_DESTRUCTOR(prio)
#else
#define CC_DESTRUCTOR(prio)
#endif /* CC_CONF_DESTRUCTOR */

/**
 * Configure if the C compiler supports marking functions as deprecated
 * e.g. with __attribute__((deprecated))
 */
#ifdef CC_CONF_DEPRECATED
#define CC_DEPRECATED(msg) CC_CONF_DEPRECATED(msg)
#else
#define CC_DEPRECATED(msg)
#endif /* CC_CONF_DEPRECATED */

/** \def CC_ACCESS_NOW(x)
 * This macro ensures that the access to a non-volatile variable can
 * not be reordered or optimized by the compiler.
 * See also https://lwn.net/Articles/508991/ - In Linux the macro is
 * called ACCESS_ONCE
 * The type must be passed, because the typeof-operator is a gcc
 * extension
 */

#define CC_ACCESS_NOW(type, variable) (*(volatile type *)&(variable))

#ifndef NULL
#define NULL 0
#endif /* NULL */

#ifndef MAX
#define MAX(n, m)   (((n) < (m)) ? (m) : (n))
#endif

#ifndef MIN
#define MIN(n, m)   (((n) < (m)) ? (n) : (m))
#endif

#ifndef ABS
#define ABS(n)      (((n) < 0) ? -(n) : (n))
#endif

#ifndef BOUND
#define BOUND(a, minimum, maximum)   MIN(MAX(a, minimum), maximum)
#endif


#define CC_CONCAT2(s1, s2) s1##s2
/**
 * A C preprocessing macro for concatenating two preprocessor tokens.
 *
 * We need use two macros (CC_CONCAT and CC_CONCAT2) in order to allow
 * concatenation of two \#defined macros.
 */
#define CC_CONCAT(s1, s2) CC_CONCAT2(s1, s2)
#define CC_CONCAT_EXT_2(s1, s2) CC_CONCAT2(s1, s2)

/**
 * A C preprocessing macro for concatenating three preprocessor tokens.
 */
#define CC_CONCAT3(s1, s2, s3) s1##s2##s3
#define CC_CONCAT_EXT_3(s1, s2, s3) CC_CONCAT3(s1, s2, s3)

#endif /* CC_H_ */

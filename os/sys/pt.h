/*
 * Copyright (c) 2004-2005, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
 * \addtogroup threads
 * @{
 */

/**
 * \defgroup pt Protothreads
 *
 * Protothreads are a type of lightweight stackless threads designed for
 * severly memory constrained systems such as deeply embedded systems or
 * sensor network nodes. Protothreads provides linear code execution for
 * event-driven systems implemented in C. Protothreads can be used with
 * or without an RTOS.
 *
 * Protothreads are a extremely lightweight, stackless type of threads
 * that provides a blocking context on top of an event-driven system,
 * without the overhead of per-thread stacks. The purpose of protothreads
 * is to implement sequential flow of control without complex state
 * machines or full multi-threading. Protothreads provides conditional
 * blocking inside C functions.
 *
 * The advantage of protothreads over a purely event-driven approach is
 * that protothreads provides a sequential code structure that allows for
 * blocking functions. In purely event-driven systems, blocking must be
 * implemented by manually breaking the function into two pieces - one
 * for the piece of code before the blocking call and one for the code
 * after the blocking call. This makes it hard to use control structures
 * such as if() conditionals and while() loops.
 *
 * The advantage of protothreads over ordinary threads is that a
 * protothread does not require a separate stack. In memory constrained
 * systems, the overhead of allocating multiple stacks can consume large
 * amounts of the available memory. In contrast, each protothread only
 * requires between two and twelve bytes of state, depending on the
 * architecture.
 *
 * \note Because protothreads do not save the stack context across a
 * blocking call, <b>local variables are not preserved when the
 * protothread blocks</b>. This means that local variables should be used
 * with utmost care - <b>if in doubt, do not use local variables inside a
 * protothread!</b>
 *
 *
 * Main features:
 *
 *  - No machine specific code - the protothreads library is pure C
 *
 *  - Does not use error-prone functions such as longjmp()
 *
 *  - Very small RAM overhead - only two bytes per protothread
 *
 *  - Can be used with or without an OS
 *
 *  - Provides blocking wait without full multi-threading or
 *    stack-switching
 *
 * Examples applications:
 *
 *  - Memory constrained systems
 *
 *  - Event-driven protocol stacks
 *
 *  - Deeply embedded systems
 *
 *  - Sensor network nodes
 *
 * The protothreads API consists of four basic operations:
 * initialization: PT_INIT(), execution: PT_BEGIN(), conditional
 * blocking: PT_WAIT_UNTIL() and exit: PT_END(). On top of these, two
 * convenience functions are built: reversed condition blocking:
 * PT_WAIT_WHILE() and protothread blocking: PT_WAIT_THREAD().
 *
 * \sa \ref pt "Protothreads API documentation"
 *
 * The protothreads library is released under a BSD-style license that
 * allows for both non-commercial and commercial usage. The only
 * requirement is that credit is given.
 *
 * \section authors Authors
 *
 * The protothreads library was written by Adam Dunkels <adam@sics.se>
 * with support from Oliver Schmidt <ol.sc@web.de>.
 *
 * \section pt-desc Protothreads
 *
 * Protothreads are a extremely lightweight, stackless threads that
 * provides a blocking context on top of an event-driven system, without
 * the overhead of per-thread stacks. The purpose of protothreads is to
 * implement sequential flow of control without using complex state
 * machines or full multi-threading. Protothreads provides conditional
 * blocking inside a C function.
 *
 * In memory constrained systems, such as deeply embedded systems,
 * traditional multi-threading may have a too large memory overhead. In
 * traditional multi-threading, each thread requires its own stack, that
 * typically is over-provisioned. The stacks may use large parts of the
 * available memory.
 *
 * The main advantage of protothreads over ordinary threads is that
 * protothreads are very lightweight: a protothread does not require its
 * own stack. Rather, all protothreads run on the same stack and context
 * switching is done by stack rewinding. This is advantageous in memory
 * constrained systems, where a stack for a thread might use a large part
 * of the available memory. A protothread only requires only two bytes of
 * memory per protothread. Moreover, protothreads are implemented in pure
 * C and do not require any machine-specific assembler code.
 *
 * A protothread runs within a single C function and cannot span over
 * other functions. A protothread may call normal C functions, but cannot
 * block inside a called function. Blocking inside nested function calls
 * is instead made by spawning a separate protothread for each
 * potentially blocking function. The advantage of this approach is that
 * blocking is explicit: the programmer knows exactly which functions
 * that block that which functions the never blocks.
 *
 * Protothreads are similar to asymmetric co-routines. The main
 * difference is that co-routines uses a separate stack for each
 * co-routine, whereas protothreads are stackless. The most similar
 * mechanism to protothreads are Python generators. These are also
 * stackless constructs, but have a different purpose. Protothreads
 * provides blocking contexts inside a C function, whereas Python
 * generators provide multiple exit points from a generator function.
 *
 * \section pt-autovars Local variables
 *
 * \note
 * Because protothreads do not save the stack context across a blocking
 * call, local variables are not preserved when the protothread
 * blocks. This means that local variables should be used with utmost
 * care - if in doubt, do not use local variables inside a protothread!
 *
 * \section pt-scheduling Scheduling
 *
 * A protothread is driven by repeated calls to the function in which the
 * protothread is running. Each time the function is called, the
 * protothread will run until it blocks or exits. Thus the scheduling of
 * protothreads is done by the application that uses protothreads.
 *
 * \section pt-impl Implementation
 *
 * Protothreads are implemented using \ref lc "local continuations". A
 * local continuation represents the current state of execution at a
 * particular place in the program, but does not provide any call history
 * or local variables. A local continuation can be set in a specific
 * function to capture the state of the function. After a local
 * continuation has been set can be resumed in order to restore the state
 * of the function at the point where the local continuation was set.
 *
 *
 * Local continuations can be implemented in a variety of ways:
 *
 * -# by using machine specific assembler code,
 * -# by using standard C constructs, or
 * -# by using compiler extensions.
 *
 * The first way works by saving and restoring the processor state,
 * except for stack pointers, and requires between 16 and 32 bytes of
 * memory per protothread. The exact amount of memory required depends on
 * the architecture.
 *
 * The standard C implementation requires only two bytes of state per
 * protothread and utilizes the C switch() statement in a non-obvious way
 * that is similar to Duff's device. This implementation does, however,
 * impose a slight restriction to the code that uses protothreads in that
 * the code cannot use switch() statements itself.
 *
 * Certain compilers has C extensions that can be used to implement
 * protothreads. GCC supports label pointers that can be used for this
 * purpose. With this implementation, protothreads require 4 bytes of RAM
 * per protothread.
 *
 * @{
 *
 * \file
 * Protothreads implementation.
 * \author
 * Adam Dunkels <adam@sics.se>
 */

#ifndef PT_H_
#define PT_H_

#include "sys/lc.h"

struct pt {
  lc_t lc;
};

#define PT_WAITING 0
#define PT_YIELDED 1
#define PT_EXITED  2
#define PT_ENDED   3

/**
 * \name Initialization
 * @{
 */

/**
 * Initialize a protothread.
 *
 * Initializes a protothread. Initialization must be done prior to
 * starting to execute the protothread.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \sa PT_SPAWN()
 *
 * \hideinitializer
 */
#define PT_INIT(pt)   LC_INIT((pt)->lc)

/** @} */

/**
 * \name Declaration and definition
 * @{
 */

/**
 * Declaration of a protothread.
 *
 * This macro is used to declare a protothread. All protothreads must
 * be declared with this macro.
 *
 * \param name_args The name and arguments of the C function
 * implementing the protothread.
 *
 * \hideinitializer
 */
#define PT_THREAD(name_args) char name_args

/**
 * Declare the start of a protothread inside the C function
 * implementing the protothread.
 *
 * This macro is used to declare the starting point of a
 * protothread. It should be placed at the start of the function in
 * which the protothread runs. All C statements above the PT_BEGIN()
 * invokation will be executed each time the protothread is scheduled.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_BEGIN(pt) { char PT_YIELD_FLAG = 1; if (PT_YIELD_FLAG) {;} LC_RESUME((pt)->lc)

/**
 * Declare the end of a protothread.
 *
 * This macro is used for declaring that a protothread ends. It must
 * always be used together with a matching PT_BEGIN() macro.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_END(pt) LC_END((pt)->lc); PT_YIELD_FLAG = 0; \
                   PT_INIT(pt); return PT_ENDED; }

/** @} */

/**
 * \name Blocked wait
 * @{
 */

/**
 * Block and wait until condition is true.
 *
 * This macro blocks the protothread until the specified condition is
 * true.
 *
 * \param pt A pointer to the protothread control structure.
 * \param condition The condition.
 *
 * \hideinitializer
 */
#define PT_WAIT_UNTIL(pt, condition)	        \
  do {						\
    LC_SET((pt)->lc);				\
    if(!(condition)) {				\
      return PT_WAITING;			\
    }						\
  } while(0)

/**
 * Block and wait while condition is true.
 *
 * This function blocks and waits while condition is true. See
 * PT_WAIT_UNTIL().
 *
 * \param pt A pointer to the protothread control structure.
 * \param cond The condition.
 *
 * \hideinitializer
 */
#define PT_WAIT_WHILE(pt, cond)  PT_WAIT_UNTIL((pt), !(cond))

/** @} */

/**
 * \name Hierarchical protothreads
 * @{
 */

/**
 * Block and wait until a child protothread completes.
 *
 * This macro schedules a child protothread. The current protothread
 * will block until the child protothread completes.
 *
 * \note The child protothread must be manually initialized with the
 * PT_INIT() function before this function is used.
 *
 * \param pt A pointer to the protothread control structure.
 * \param thread The child protothread with arguments
 *
 * \sa PT_SPAWN()
 *
 * \hideinitializer
 */
#define PT_WAIT_THREAD(pt, thread) PT_WAIT_WHILE((pt), PT_SCHEDULE(thread))

/**
 * Spawn a child protothread and wait until it exits.
 *
 * This macro spawns a child protothread and waits until it exits. The
 * macro can only be used within a protothread.
 *
 * \param pt A pointer to the protothread control structure.
 * \param child A pointer to the child protothread's control structure.
 * \param thread The child protothread with arguments
 *
 * \hideinitializer
 */
#define PT_SPAWN(pt, child, thread)		\
  do {						\
    PT_INIT((child));				\
    PT_WAIT_THREAD((pt), (thread));		\
  } while(0)

/** @} */

/**
 * \name Exiting and restarting
 * @{
 */

/**
 * Restart the protothread.
 *
 * This macro will block and cause the running protothread to restart
 * its execution at the place of the PT_BEGIN() call.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_RESTART(pt)				\
  do {						\
    PT_INIT(pt);				\
    return PT_WAITING;			\
  } while(0)

/**
 * Exit the protothread.
 *
 * This macro causes the protothread to exit. If the protothread was
 * spawned by another protothread, the parent protothread will become
 * unblocked and can continue to run.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_EXIT(pt)				\
  do {						\
    PT_INIT(pt);				\
    return PT_EXITED;			\
  } while(0)

/** @} */

/**
 * \name Calling a protothread
 * @{
 */

/**
 * Schedule a protothread.
 *
 * This function schedules a protothread. The return value of the
 * function is non-zero if the protothread is running or zero if the
 * protothread has exited.
 *
 * \param f The call to the C function implementing the protothread to
 * be scheduled
 *
 * \hideinitializer
 */
#define PT_SCHEDULE(f) ((f) < PT_EXITED)

/** @} */

/**
 * \name Yielding from a protothread
 * @{
 */

/**
 * Yield from the current protothread.
 *
 * This function will yield the protothread, thereby allowing other
 * processing to take place in the system.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_YIELD(pt)				\
  do {						\
    PT_YIELD_FLAG = 0;				\
    LC_SET((pt)->lc);				\
    if(PT_YIELD_FLAG == 0) {			\
      return PT_YIELDED;			\
    }						\
  } while(0)

/**
 * \brief      Yield from the protothread until a condition occurs.
 * \param pt   A pointer to the protothread control structure.
 * \param cond The condition.
 *
 *             This function will yield the protothread, until the
 *             specified condition evaluates to true.
 *
 *
 * \hideinitializer
 */
#define PT_YIELD_UNTIL(pt, cond)		\
  do {						\
    PT_YIELD_FLAG = 0;				\
    LC_SET((pt)->lc);				\
    if((PT_YIELD_FLAG == 0) || !(cond)) {	\
      return PT_YIELDED;			\
    }						\
  } while(0)

/** @} */

#endif /* PT_H_ */

/**
 * @}
 * @}
 */

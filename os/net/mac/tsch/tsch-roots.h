/*
 * Copyright (c) 2018, Amber Agriculture
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
 */
/**
 * \author Atis Elsts <atis.elsts@gmail.com>
 */

#ifndef TSCH_ROOTS_H_
#define TSCH_ROOTS_H_

/**
 * Add address as a potential RPL root that is a single-hop neighbor in the TSCH network.
 * For now, only add addresses that send TSCH EB's with join priority set to zero.
 *
 * \param    root_address - the address of the root.
 */
void tsch_roots_add_address(const linkaddr_t *root_address);

/**
 * Set the root status of the local node
 *
 * \param    is_root - if nonzero, the local node becomes root; otherwise, it ceases to be root
 */
void tsch_roots_set_self_to_root(uint8_t is_root);

/**
 * Tests whether a given address belongs to a single-hop reachable root node in this network.
 *
 * \param    address - the address to test
 * \return   nonzero if the address belongs to a root node that is a single hop neighbor;
 *           zero otherwise.
 *
 * Note: does not check if the address belongs to self, or to a root node more than a hop away!
 */
int tsch_roots_is_root(const linkaddr_t *address);

/**
 * Initialize the list of RPL network roots.
 */
void tsch_roots_init(void);

#endif /* TSCH_ROOTS_H_ */

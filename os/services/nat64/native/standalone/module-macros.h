/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
 *
 */

/*
 * Built into every translation unit via -imacros, so a node becomes a
 * standalone NAT64 translator just by building the module:
 *
 *  - Route the node's routeless (off-link) traffic through the standalone
 *    fallback interface, so NAT64-prefix destinations reach the translator.
 *  - Drop the platform's default route to PREFIX::1 (platform.c). With no
 *    border router it is unreachable, and as a default route it would shadow
 *    the fallback interface.
 *  - Enable NAT64 without the --nat64 flag (nat64-sock.c). Building the module
 *    already says "this node is a NAT64 translator", and it keeps enablement a
 *    compile-time choice that matches the route suppression above. Otherwise a
 *    build without the flag would have neither a default route nor NAT64, and
 *    could reach nothing off-link.
 */
#define UIP_FALLBACK_INTERFACE nat64_standalone_interface
#define NATIVE_WITH_IPV6_DEFAULT_ROUTE 0
#define NAT64_DEFAULT_ENABLED 1

/*
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-5340-application nRF5340 Application Core
 * @{
 *
 * \file
 *      Header with configuration defines to nrf 5340 application core
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 */
/*---------------------------------------------------------------------------*/
#ifndef NRF5340_APPLICATION_CONF_H_
#define NRF5340_APPLICATION_CONF_H_
/*---------------------------------------------------------------------------*/
/*
 * Enable the CPU code/data cache at boot. The application core executes
 * from wait-stated flash behind the CACHE peripheral, which is disabled
 * at reset. Set 0 to run uncached, e.g. for cycle-exact profiling. With
 * TrustZone the cache is a secure-only peripheral, so this must be set
 * when building the secure world; the normal world cannot change it. Do
 * this with `make TRUSTZONE=1 DEFINES=NRF_CONF_ICACHE_ENABLE=0`, since an
 * application's own project-conf.h does not reach the recursively-built
 * secure world.
 */
#ifndef NRF_CONF_ICACHE_ENABLE
#define NRF_CONF_ICACHE_ENABLE 1
#endif
/*---------------------------------------------------------------------------*/
#endif /* NRF5340_APPLICATION_CONF_H_ */
/*---------------------------------------------------------------------------*/
/** 
 * @}
 * @}
 */

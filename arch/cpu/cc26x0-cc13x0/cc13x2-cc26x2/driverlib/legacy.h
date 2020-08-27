/*
 * Copyright (c) 2020, alexrayne <alexraynepe196@gmail.com>
 * All rights reserved.
 *
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

/*
 * this is wrapper header for emulate obsolete chipinfo for cc13x0 target
 * */

//*****************************************************************************
//
//! \addtogroup system_control_group
//! @{
//! \addtogroup ChipInfo
//! @{
//
//*****************************************************************************

#ifndef __CHIP_INFO_LEGACY_H__
#define __CHIP_INFO_LEGACY_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "driverlib/chipinfo.h"

#ifdef THIS_DRIVERLIB_BUILD
#if THIS_DRIVERLIB_BUILD == DRIVERLIB_BUILD_CC13X0
#define ChipInfo_ChipFamilyIsCC13xx() 1
#define ChipInfo_ChipFamilyIsCC26xx() 0
#elif (THIS_DRIVERLIB_BUILD == DRIVERLIB_BUILD_CC26X0) \
    | (THIS_DRIVERLIB_BUILD == DRIVERLIB_BUILD_CC26X1) \
    | (THIS_DRIVERLIB_BUILD == DRIVERLIB_BUILD_CC26X0R2)
#define ChipInfo_ChipFamilyIsCC13xx() 0
#define ChipInfo_ChipFamilyIsCC26xx() 1
#else
bool ChipInfo_ChipFamilyIsCC13xx( void );
bool ChipInfo_ChipFamilyIsCC26xx( void );
#endif

#else
bool ChipInfo_ChipFamilyIsCC13xx( void );
bool ChipInfo_ChipFamilyIsCC26xx( void );
#endif



//------------------------------------------------------------------------------
static inline
void RFCAdi3VcoLdoVoltageMode(bool bEnable){}


//------------------------------------------------------------------------------
#define PRCM_UARTCLKGR_CLK_EN   PRCM_UARTCLKGR_CLK_EN_UART0

//*****************************************************************************
#define AON_EVENT_AUX_WU0   AON_EVENT_MCU_WU4
#define AON_EVENT_AUX_WU1   AON_EVENT_MCU_WU5
#define AON_EVENT_AUX_WU2   AON_EVENT_MCU_WU6

#define AONEventAuxWakeUpSet(...) AONEventMcuSet(__VA_ARGS__)
#define AONEventAuxWakeUpGet(...) AONEventMcuGet(__VA_ARGS__)


//==============================================================================
//  new cc13/26x2 API
//------------------------------------------------------------------------------
#include "driverlib/aux_sysif.h"

#define ti_lib_aux_sysif_opmode_change(...)   AUXSYSIFOpModeChange(__VA_ARGS__)

#include "driverlib/sys_ctrl.h"
#define ti_lib_sys_ctrl_shutdown_with_abort(...)  SysCtrlShutdownWithAbort(__VA_ARGS__)


#ifndef PRCMPowerDomainsAllOn
#define PRCMPowerDomainsAllOn(...) PRCMPowerDomainStatus(__VA_ARGS__)
#endif

#ifndef PRCMPowerDomainsAllOff

//*****************************************************************************
//
//! \brief Get the status for a specific power domain.
//!
//! Use this function to retrieve the current power status of one or more
//! power domains.
//!
//! \param ui32Domains determines which domain to get the power status for.
//! The parameter must be an OR'ed combination of one or several of:
//! - \ref PRCM_DOMAIN_RFCORE : RF Core.
//! - \ref PRCM_DOMAIN_SERIAL : SSI0, UART0, I2C0
//! - \ref PRCM_DOMAIN_PERIPH : GPT0, GPT1, GPT2, GPT3, GPIO, SSI1, I2S, DMA, UART1
//!
//! \return Returns status of the requested domains:
//! - \ref PRCM_DOMAIN_POWER_OFF : The specified domains are \b all powered down.
//! This status is unconditional and the powered down status is guaranteed.
//! - \ref PRCM_DOMAIN_POWER_OFF : Any of the domains are still powered up.
//
//*****************************************************************************
uint32_t ti_lib_PRCMPowerDomainsAllOff(uint32_t ui32Domains);

#define PRCMPowerDomainsAllOff(...) ti_lib_PRCMPowerDomainsAllOff(__VA_ARGS__)

#endif



#ifdef __cplusplus
}
#endif

#endif // __CHIP_INFO_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************

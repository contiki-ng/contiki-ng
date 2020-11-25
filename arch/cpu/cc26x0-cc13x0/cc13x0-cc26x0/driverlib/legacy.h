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



//=============================================================================
//                      appear since 2018
//-----------------------------------------------------------------------------
#include "driverlib/prcm.h"

#ifndef PRCM_O_RFCMODEHWOPT
// Allowed RFC Modes
#define PRCM_O_RFCMODEHWOPT     (PRCM_O_RFCMODESEL+4)
#endif

//-----------------------------------------------------------------------------
#include "driverlib/rfc.h"

#ifndef RFCOverrideUpdate
#define RFCOverrideUpdate(pOpSetup, pParams) RFCRTrim(pOpSetup)
#endif


#include <inc/hw_aon_sysctl.h>

#if defined(DRIVERLIB_BUILD_CC13X2_CC26X2)
// new driverlib have deprecate this

#include "driverlib/aon_ioc.h"

//*****************************************************************************
//
//! \brief Close the latches in the AON IOC interface and in padring.
//!
//! Use this function to unfreeze the current value retained on the IOs driven
//! by the device. This is required if it is desired to maintain the level of
//! any IO driven when going through a shutdown/powerdown cycle.
//!
//! \return None
//!
//! \sa \ref PowerCtrlIOFreezeDisable()
//
//*****************************************************************************
__STATIC_INLINE
void PowerCtrlIOFreezeEnable(void)
{
    //
    // Close the IO latches at AON_IOC level and in the padring.
    //
    AONIOCFreezeEnable();
    HWREG(AON_SYSCTL_BASE + AON_SYSCTL_O_SLEEPCTL) = 0;
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
}

//*****************************************************************************
//
//! Open the latches in the AON IOC interface and in padring.
//!
//! Use this function to unfreeze the latches that retained on the IOs driven
//! by the device. This function should not be called before the application
//! has reinitialized the IO configuration that will drive the IOs to the
//! desired level.
//!
//! \return None
//!
//! \sa \ref PowerCtrlIOFreezeEnable()
//
//*****************************************************************************
__STATIC_INLINE
void PowerCtrlIOFreezeDisable(void)
{
    //
    // Open the IO latches at AON_IOC level and in the padring
    //
    AONIOCFreezeDisable();
    HWREG(AON_SYSCTL_BASE + AON_SYSCTL_O_SLEEPCTL) = 1;
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
}

#else
// new driverlib establish this

//*****************************************************************************
//
//! \brief Enables pad sleep in order to latch device outputs before shutdown.
//!
//! \return None
//!
//! \sa \ref PowerCtrlPadSleepDisable()
//
//*****************************************************************************
__STATIC_INLINE
void PowerCtrlPadSleepEnable(void)
{
    HWREG(AON_SYSCTL_BASE + AON_SYSCTL_O_SLEEPCTL) = 0;
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
}

//*****************************************************************************
//
//! \brief Disables pad sleep in order to unlatch device outputs after wakeup from shutdown.
//!
//! This function must be called by the application after the device wakes up
//! from shutdown.
//!
//! \return None
//!
//! \sa \ref PowerCtrlPadSleepEnable()
//
//*****************************************************************************
__STATIC_INLINE
void PowerCtrlPadSleepDisable(void)
{
    HWREG(AON_SYSCTL_BASE + AON_SYSCTL_O_SLEEPCTL) = 1;
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
}

#endif

//==============================================================================
// new API cc13/26x2 compatibily
#define PRCM_UARTCLKGR_CLK_EN_UART0     PRCM_UARTCLKGR_CLK_EN

//------------------------------------------------------------------------------
#include "driverlib/sys_ctrl.h"
//*****************************************************************************
//
// Defines for the vimsPdMode parameter of SysCtrlIdle and SysCtrlStandby
//
//*****************************************************************************
#define VIMS_ON_CPU_ON_MODE     0 // VIMS power domain is only powered when CPU power domain is powered
#define VIMS_ON_BUS_ON_MODE     1 // VIMS power domain is powered whenever the BUS power domain is powered
#define VIMS_NO_PWR_UP_MODE     2 // VIMS power domain is not powered up at next wakeup.
void SysCtrlIdle(uint32_t vimsPdMode);

// Try to enter shutdown but abort if wakeup event happened before shutdown
void SysCtrlShutdownWithAbort(void);
void SysCtrlShutdown(void);

//< disables switch to uLDO in deepsleep
#define STANDBY_MCUPWR_NOULDO 0x10000
//< Defines for the rechargeMode parameter of SysCtrlStandby, default one
#define SYSCTRL_PREFERRED_RECHARGE_MODE     0xFFFFFFFF

void SysCtrlStandby(bool retainCache, uint32_t vimsPdMode, uint32_t rechargeMode);

#define ti_lib_sys_ctrl_shutdown_with_abort(...)  SysCtrlShutdownWithAbort(__VA_ARGS__)



// this new PRCM API introduces in 2020y
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
//! - \sa PRCM_DOMAIN_RFCORE : RF Core.
//! - \sa PRCM_DOMAIN_SERIAL : SSI0, UART0, I2C0
//! - \sa PRCM_DOMAIN_PERIPH : GPT0, GPT1, GPT2, GPT3, GPIO, SSI1, I2S, DMA, UART1
//!
//! \return Returns status of the requested domains:
//! - \sa PRCM_DOMAIN_POWER_OFF : The specified domains are \b all powered down.
//! This status is unconditional and the powered down status is guaranteed.
//! - \sa PRCM_DOMAIN_POWER_OFF : Any of the domains are still powered up.
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

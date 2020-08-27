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
 * this is wrapper header for emulate obsolete auxwuc_api for cc13x2 target
 * */

//****************************************************************************
//
//! \addtogroup aux_group
//! @{
//! \addtogroup auxwuc_api
//! @{
//
//****************************************************************************

#ifndef __AON_WUC_H__
#define __AON_WUC_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "driverlib/aon_pmctl.h"

#define MCU_RAM0_RETENTION      0x00000001
#define MCU_RAM1_RETENTION      0x00000002
#define MCU_RAM2_RETENTION      0x00000004
#define MCU_RAM3_RETENTION      0x00000008
#define MCU_RAM_BLOCK_RETENTION 0x0000000F
#define AONWUCMcuSRamConfig(...)    AONPMCTLMcuSRamRetConfig(__VA_ARGS__)

//! \param ui32Retention either enables or disables AUX SRAM retention.
//! - 0 : Disable retention.
//! - 1 : Enable retention.
__STATIC_INLINE
void AONWUCAuxSRamConfig(int bRetention)
{
    // Enable/disable the retention.
    if (bRetention)
        HWREG(AON_PMCTL_BASE + AON_PMCTL_O_RAMCFG) |= AON_PMCTL_RAMCFG_AUX_SRAM_RET_EN;
    else
        HWREG(AON_PMCTL_BASE + AON_PMCTL_O_RAMCFG) &= ~AON_PMCTL_RAMCFG_AUX_SRAM_RET_EN;
}


//AONWUC_AUX_POWER_ON

#define AONWUC_AUX_POWER_DOWN       0x00000200  // AUX is in powerdown mode
#define AONWUC_AUX_POWER_ON         0x00000020  // AUX is powered on
#define AONWUC_JTAG_POWER_ON        AONPMCTL_JTAG_POWER_ON
//*****************************************************************************
//
//! \brief Get the power status of the device.
//!
//! The Always On (AON) domain is the only part of the device which is truly
//! "ALWAYS ON". The power status for the other device can always be read from
//! this status register.
//!
//! Possible power modes for the different parts of the device are:
//!
//! \return Returns the current power status of the device as a bitwise OR'ed
//! combination of these values:
//! - \ref AONWUC_AUX_POWER_DOWN
//! - \ref AONWUC_AUX_POWER_ON
//! - \ref AONWUC_JTAG_POWER_ON
//
//*****************************************************************************
uint32_t AONWUCPowerStatusGet(void);

#define AONWUCJtagPowerOff(...)     AONPMCTLJtagPowerOff(__VA_ARGS__)



#define AON_SYSCTL_BASE             AON_PMCTL_BASE
#define AON_SYSCTL_O_PWRCTL         AON_PMCTL_O_PWRCTL
#define AON_SYSCTL_O_RESETCTL       AON_PMCTL_O_RESETCTL
#define AON_SYSCTL_O_SLEEPCTL       AON_PMCTL_O_SLEEPCTL



//*****************************************************************************
//
// Defines the possible clock source for the MCU and AUX domain.
//
//*****************************************************************************
//#define AONWUC_CLOCK_SRC_HF     0x00000003  // System clock high frequency -
                                            // 48 MHz.
#define AONWUC_CLOCK_SRC_LF     0x00000001  // System clock low frequency -
                                            // 32 kHz.
#define AONWUC_NO_CLOCK         0x00000000  // System clock low frequency -
                                            // 32 kHz.
//*****************************************************************************
//
//! \brief Configure the power down mode for the AUX domain.
//!
//! Use this function to control which one of the clock sources that
//! is fed into the MCU domain when it is in Power Down mode. When the Power
//! is back in active mode the clock source will automatically switch to
//! \ref AONWUC_CLOCK_SRC_HF.
//!
//! Each clock is fed 'as is' into the AUX domain, since the AUX domain
//! contains internal clock dividers controllable through the PRCM.
//!
//! \param ui32ClkSrc is the clock source for the AUX domain when in power down.
//! - \ref AONWUC_NO_CLOCK
//! - \ref AONWUC_CLOCK_SRC_LF
//! \return None
//
__STATIC_INLINE
void AONWUCAuxPowerDownConfig(uint32_t ui32ClkSrc)
{
    // Check the arguments.
    ASSERT((ui32ClkSrc == AONWUC_NO_CLOCK) ||
           (ui32ClkSrc == AONWUC_CLOCK_SRC_LF));

    // Set the clock source for the AUX domain when in power down.
    if (ui32ClkSrc != AONWUC_NO_CLOCK) // ==AONWUC_CLOCK_SRC_LF
        HWREG(AON_PMCTL_BASE + AON_PMCTL_O_AUXSCECLK) |= AON_PMCTL_AUXSCECLK_PD_SRC;
    else
        HWREG(AON_PMCTL_BASE + AON_PMCTL_O_AUXSCECLK) &= ~AON_PMCTL_AUXSCECLK_PD_SRC;
}


#ifdef __cplusplus
}
#endif

#endif //  __AON_WUC_H__

//****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//****************************************************************************

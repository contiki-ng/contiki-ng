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

#ifndef __AUX_WUC_H__
#define __AUX_WUC_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "driverlib/aux_sysif.h"

// cc13x2 platform have IOC in AON power domain, no need freeze
#define AUXWUCFreezeDisable(...)
#define AUXWUCFreezeEnable(...)


// cc13x2 platform have no AUX clock control, so provide this for cc13/26x0 compatibily
#define AUX_WUC_SMPH_CLOCK      (1 << 0)
#define AUX_WUC_AIODIO0_CLOCK   (1 << 1)
#define AUX_WUC_AIODIO1_CLOCK   (1 << 2)
#define AUX_WUC_TIMER_CLOCK     (1 << 3)
#define AUX_WUC_ANAIF_CLOCK     (1 << 4)
#define AUX_WUC_TDCIF_CLOCK     (1 << 5)
//#define AUX_WUC_OSCCTRL_CLOCK   (1 << 6)
#define AUX_WUC_OSCCTRL_CLOCK   0
#define AUX_WUC_ADI_CLOCK       (1 << 7)
#define AUX_WUC_MODCLK_MASK     0x000000FF

#define AUXWUCClockEnable(ui32Clocks)
#define AUXWUCClockDisable(ui32Clocks)



#define AUX_WUC_CLOCK_OFF       0x00000000
#define AUX_WUC_CLOCK_UNSTABLE  0x00000001
#define AUX_WUC_CLOCK_READY     0x00000011

// cc13/26x2 aux clockes always on
#define AUXWUCClockStatus(ui32Clocks)   AUX_WUC_CLOCK_READY



//*****************************************************************************
//
//! @brief Defines for the AUX power control.
//
//*****************************************************************************
#define AUX_WUC_POWER_OFF       0x00000001 //!< same as POWER_DOWN \ref AUX_WUC_POWER_DOWN
#define AUX_WUC_POWER_DOWN      0x00000002 //!< AUX go to power down
#define AUX_WUC_POWER_ACTIVE    0x00000004 //!< AUX go to active state

//****************************************************************************
//
//! \brief Control the power to the AUX domain.
//!
//! Use this function to set the power mode of the entire AUX domain.
//!
//! \param ui32PowerMode control the desired power mode for the AUX domain.
//! The domain has three different power modes:
//! - \ref AUX_WUC_POWER_OFF
//! - \ref AUX_WUC_POWER_DOWN
//! - \ref AUX_WUC_POWER_ACTIVE
//!
//! \return None
//!
//! \note cc13/26x2 have AUX power : active, lowpower <-> powerdown
//!         here no use LowPower mode, for code simplify. Possibly it is work TODO.
//!
//
//****************************************************************************
__STATIC_INLINE
void AUXWUCPowerCtrl(uint32_t ui32PowerMode){
    if (ui32PowerMode == AUX_WUC_POWER_ACTIVE)
        AUXSYSIFOpModeChange(AUX_SYSIF_OPMODE_TARGET_A);
    else
        // cc13/26x2 AUX alwayw powered, so AUX_WUC_POWER_OFF is same as DOWN
        AUXSYSIFOpModeChange(AUX_SYSIF_OPMODE_TARGET_PDA);
}

#define AONWUC_AUX_WAKEUP       0x00000001
// keep it for cc13x0 compatibily
#define AONWUC_AUX_ALLOW_SLEEP  0x00000000

//! \brief the wake up procedure of the AUX domain.
__STATIC_INLINE
void AONWUCAuxWakeupEvent(uint32_t ui32Mode){
    if (ui32Mode == AONWUC_AUX_WAKEUP)
        AUXSYSIFOpModeChange(AUX_SYSIF_OPMODE_TARGET_A);
}



#ifdef __cplusplus
}
#endif

#endif //  __AUX_WUC_H__

//****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//****************************************************************************

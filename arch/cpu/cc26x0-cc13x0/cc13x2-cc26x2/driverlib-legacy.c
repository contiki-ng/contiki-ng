#include "driverlib/legacy.h"


const unsigned types_cc13xx = 1<<CHIP_TYPE_CC1310
                            | 1<<CHIP_TYPE_CC1350
                            | 1<<CHIP_TYPE_CC1312
                            | 1<<CHIP_TYPE_CC1352
                            | 1<<CHIP_TYPE_CC1352P
                            ;
const unsigned types_cc26xx = 1<<CHIP_TYPE_CC2620
                            | 1<<CHIP_TYPE_CC2630
                            | 1<<CHIP_TYPE_CC2640
                            | 1<<CHIP_TYPE_CC2650
                            | 1<<CHIP_TYPE_CC2640R2
                            | 1<<CHIP_TYPE_CC2642
                            | 1<<CHIP_TYPE_CC2652
                            ;
#ifndef ChipInfo_ChipFamilyIsCC13xx
bool ChipInfo_ChipFamilyIsCC13xx( void ){
    return ((types_cc13xx >> ChipInfo_GetChipType())&1) != 0 ;
}

#endif //ChipInfo_ChipFamilyIsCC13xx

#ifndef ChipInfo_ChipFamilyIsCC26xx
bool ChipInfo_ChipFamilyIsCC26xx( void ){
    return ((types_cc26xx >> ChipInfo_GetChipType())&1) != 0 ;
}
#endif //ChipInfo_ChipFamilyIsCC26xx



#include "driverlib/aon_wuc.h"
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
uint32_t AONWUCPowerStatusGet(void) {
    uint32_t x;

    uint32_t auxmode = HWREG(AUX_SYSIF_BASE + AUX_SYSIF_O_OPMODEACK);
    if (auxmode & AUX_SYSIF_OPMODEACK_ACK_PDA)
        x = AONWUC_AUX_POWER_DOWN;
    else
        x = AONWUC_AUX_POWER_ON;

    x |= AONPMCTLPowerStatusGet();
    return x;
}


//*****************************************************************************
//
// Return PRCM_DOMAIN_POWER_OFF if all power domains are off
//
//*****************************************************************************
uint32_t ti_lib_PRCMPowerDomainsAllOff(uint32_t ui32Domains)
{
    bool bStatus;
    uint32_t ui32StatusRegister0;
    uint32_t ui32StatusRegister1;

    // Check the arguments.
    ASSERT((ui32Domains & (PRCM_DOMAIN_RFCORE |
                           PRCM_DOMAIN_SERIAL |
                           PRCM_DOMAIN_PERIPH)));

    bStatus = false;
    ui32StatusRegister0 = HWREG(PRCM_BASE + PRCM_O_PDSTAT0);
    ui32StatusRegister1 = HWREG(PRCM_BASE + PRCM_O_PDSTAT1);

    // Return the correct power status.
    if(ui32Domains & PRCM_DOMAIN_RFCORE)
    {
       bStatus = bStatus ||
                 ((ui32StatusRegister0 & PRCM_PDSTAT0_RFC_ON) ||
                  (ui32StatusRegister1 & PRCM_PDSTAT1_RFC_ON));
    }
    if(ui32Domains & PRCM_DOMAIN_SERIAL)
    {
        bStatus = bStatus || (ui32StatusRegister0 & PRCM_PDSTAT0_SERIAL_ON);
    }
    if(ui32Domains & PRCM_DOMAIN_PERIPH)
    {
        bStatus = bStatus || (ui32StatusRegister0 & PRCM_PDSTAT0_PERIPH_ON);
    }

    // Return the status.
    return (bStatus ? PRCM_DOMAIN_POWER_ON : PRCM_DOMAIN_POWER_OFF);
}

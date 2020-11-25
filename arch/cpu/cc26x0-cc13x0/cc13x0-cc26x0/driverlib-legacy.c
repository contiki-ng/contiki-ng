#include "driverlib/legacy.h"

#ifndef DRIVERLIB_BUILD_CC13X2_CC26X2
enum {
    CHIP_TYPE_CC2640R2      =  8, //!<  8 means that this is a CC2640R2 chip.
    CHIP_TYPE_CC2642        =  9, //!<  9 means that this is a CC2642 chip.
    CHIP_TYPE_unused        =  10,//!< 10 unused value
    CHIP_TYPE_CC2652        =  11,//!< 11 means that this is a CC2652 chip.
    CHIP_TYPE_CC1312        =  12,//!< 12 means that this is a CC1312 chip.
    CHIP_TYPE_CC1352        =  13,//!< 13 means that this is a CC1352 chip.
    CHIP_TYPE_CC1352P       =  14 //!< 14 means that this is a CC1352P chip.
};
#endif

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

#ifdef DRIVERLIB_BUILD_CC13X2_CC26X2

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

#endif // defined DRIVERLIB_BUILD_CC13X2_CC26X2


//==============================================================================
#include "dev/aux-ctrl.h"
#include "dev/oscillators.h"


//*****************************************************************************
//
// Force the system in to idle mode
//
//*****************************************************************************
void SysCtrlIdle(uint32_t vimsPdMode)
{
    // Configure the VIMS mode
    HWREG(PRCM_BASE + PRCM_O_PDCTL1VIMS) = vimsPdMode;

    // Always keep cache retention ON in IDLE
    PRCMCacheRetentionEnable();

    // Turn off the CPU power domain, will take effect when PRCMDeepSleep() executes
    PRCMPowerDomainOff(PRCM_DOMAIN_CPU);

    // Ensure any possible outstanding AON writes complete
    SysCtrlAonSync();

    // Invoke deep sleep to go to IDLE
    PRCMDeepSleep();
}

//*****************************************************************************
//
// Try to enter shutdown but abort if wakeup event happened before shutdown
//
//*****************************************************************************
void SysCtrlShutdownWithAbort(void)
{

    uint32_t wu_detect_vector = 0;
    uint32_t io_num = 0;

    // For all IO CFG registers check if wakeup detect is enabled
    for(io_num = 0; io_num < 32; io_num++)
    {
        // Read MSB from WU_CFG bit field
        if( HWREG(IOC_BASE + IOC_O_IOCFG0 + (io_num * 4) ) & (1 << (IOC_IOCFG0_WU_CFG_S + IOC_IOCFG0_WU_CFG_W - 1)) )
        {
            wu_detect_vector |= (1 << io_num);
        }
    }

    // Wakeup events are detected when pads are in sleep mode
    PowerCtrlPadSleepEnable();

    // Make sure all potential events have propagated before checking event flags
    SysCtrlAonUpdate();
    SysCtrlAonUpdate();

    // If no edge detect flags for wakeup enabled IOs are set then shut down the device
    if( GPIO_getEventMultiDio(wu_detect_vector) == 0 )
    {

        /* Freeze I/O latches in AON */
        ti_lib_aon_ioc_freeze_enable();

        /* Turn off RFCORE, SERIAL and PERIPH PDs. This will happen immediately */
        ti_lib_prcm_power_domain_off(PRCM_DOMAIN_RFCORE | PRCM_DOMAIN_SERIAL |
                                     PRCM_DOMAIN_PERIPH);

        aux_consumer_module_t aux = { .clocks = AUX_WUC_OSCCTRL_CLOCK };
        /* Register an aux-ctrl consumer to avoid powercycling AUX twice in a row */
        aux_ctrl_register_consumer(&aux);
        oscillators_switch_to_hf_rc();
        oscillators_select_lf_rcosc();

        /* Configure clock sources for MCU: No clock */
        ti_lib_aon_wuc_mcu_power_down_config(AONWUC_NO_CLOCK);

        /* Disable SRAM retention */
        ti_lib_aon_wuc_mcu_sram_config(0);

        /*
         * Request CPU, SYSBYS and VIMS PD off.
         * This will only happen when the CM3 enters deep sleep
         */
        ti_lib_prcm_power_domain_off(PRCM_DOMAIN_CPU | PRCM_DOMAIN_VIMS |
                                     PRCM_DOMAIN_SYSBUS);

        /* Request JTAG domain power off */
        ti_lib_aon_wuc_jtag_power_off();

        /* Turn off AUX */
        aux_ctrl_power_down(true);
        ti_lib_aon_wuc_domain_power_down_enable();

        /*
         * Request MCU VD power off.
         * This will only happen when the CM3 enters deep sleep
         */
        ti_lib_prcm_mcu_power_off();

        /* Set MCU wakeup to immediate and disable virtual power off */
        ti_lib_aon_wuc_mcu_wake_up_config(MCU_IMM_WAKE_UP);
        ti_lib_aon_wuc_mcu_power_off_config(MCU_VIRT_PWOFF_DISABLE);

        /* Latch the IOs in the padring and enable I/O pad sleep mode */
        ti_lib_aon_ioc_freeze_enable();
        HWREG(AON_SYSCTL_BASE + AON_SYSCTL_O_SLEEPCTL) = 0;
        ti_lib_sys_ctrl_aon_sync();

        /* Turn off VIMS cache, CRAM and TRAM - possibly not required */
        ti_lib_prcm_cache_retention_disable();
        ti_lib_vims_mode_set(VIMS_BASE, VIMS_MODE_OFF);

        SysCtrlShutdown();
    }
    else
    {
        PowerCtrlPadSleepDisable();
    }
}

//*****************************************************************************
//
// Force the system into shutdown mode
//
//*****************************************************************************
void SysCtrlShutdown(void)
{
    /* Enable shutdown and sync AON */
    AONWUCShutDownEnable();
    SysCtrlAonSync();

    /* Deep Sleep */
    PRCMDeepSleep();

    // Make sure System CPU does not continue beyond this point.
    // Shutdown happens when all shutdown conditions are met.
    while(1);
}

//*****************************************************************************
//
// Force the system in to standby mode
//
//*****************************************************************************
void SysCtrlStandby(bool retainCache, uint32_t vimsPdMode, uint32_t rechargeMode)
{
    /* Shut Down the AUX if the user application is not using it */
    aux_ctrl_power_down(false);

    /* Configure clock sources for MCU: No clock */
    ti_lib_aon_wuc_mcu_power_down_config(AONWUC_NO_CLOCK);

    /* Full RAM retention. */
    ti_lib_aon_wuc_mcu_sram_config(MCU_RAM0_RETENTION | MCU_RAM1_RETENTION |
                                   MCU_RAM2_RETENTION | MCU_RAM3_RETENTION);

    // Gate running deep sleep clocks for Crypto, DMA
    PRCMPeripheralDeepSleepDisable(PRCM_PERIPH_CRYPTO);
    PRCMPeripheralDeepSleepDisable(PRCM_PERIPH_UDMA);
    /* 7. Make sure clock settings take effect */
    PRCMLoadSet();

    /*
     * Always turn off RFCORE, CPU, SYSBUS and VIMS. RFCORE should be off
     * already
     */
    ti_lib_prcm_power_domain_off(PRCM_DOMAIN_RFCORE | PRCM_DOMAIN_CPU |
                                 PRCM_DOMAIN_VIMS | PRCM_DOMAIN_SYSBUS);

    /* Request JTAG domain power off */
    ti_lib_aon_wuc_jtag_power_off();

    // Configure the VIMS power domain mode
    HWREG(PRCM_BASE + PRCM_O_PDCTL1VIMS) = vimsPdMode;

    /* Allow MCU and AUX powerdown */
    ti_lib_aon_wuc_domain_power_down_enable();

    /* Configure the recharge controller */
    ti_lib_sys_ctrl_set_recharge_before_power_down(XOSC_IN_HIGH_POWER_MODE);
    /*
     * If both PERIPH and SERIAL PDs are off, request the uLDO as the power
     * source while in deep sleep.
     */
    if((vimsPdMode && STANDBY_MCUPWR_NOULDO) == 0) {

      ti_lib_pwr_ctrl_source_set(PWRCTRL_PWRSRC_ULDO);
    }

    /* Sync the AON interface to ensure all writes have gone through. */
    ti_lib_sys_ctrl_aon_sync();

    /*
     * Explicitly turn off VIMS cache, CRAM and TRAM. Needed because of
     * retention mismatch between VIMS logic and cache. We wait to do this
     * until right before deep sleep to be able to use the cache for as long
     * as possible.
     */
    ti_lib_prcm_cache_retention_disable();
    ti_lib_vims_mode_set(VIMS_BASE, VIMS_MODE_OFF);

    /* Deep Sleep */
    ti_lib_prcm_deep_sleep();
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

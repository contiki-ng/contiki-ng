/// \addtogroup module_scif_generic_interface
//@{
#include "scif_framework.h"
#undef DEVICE_FAMILY_PATH
#if defined(DEVICE_FAMILY)
    #define DEVICE_FAMILY_PATH(x) <ti/devices/DEVICE_FAMILY/x>
#elif defined(DeviceFamily_CC26X0)
    #define DEVICE_FAMILY_PATH(x) <ti/devices/cc26x0/x>
#elif defined(DeviceFamily_CC26X0R2)
    #define DEVICE_FAMILY_PATH(x) <ti/devices/cc26x0r2/x>
#elif defined(DeviceFamily_CC13X0)
    #define DEVICE_FAMILY_PATH(x) <ti/devices/cc13x0/x>
#else
    #define DEVICE_FAMILY_PATH(x) <x>
#endif
#include DEVICE_FAMILY_PATH(inc/hw_types.h)
#include DEVICE_FAMILY_PATH(inc/hw_memmap.h)
#include DEVICE_FAMILY_PATH(inc/hw_aon_event.h)
#include DEVICE_FAMILY_PATH(inc/hw_aon_rtc.h)
#include DEVICE_FAMILY_PATH(inc/hw_aon_wuc.h)
#include DEVICE_FAMILY_PATH(inc/hw_aux_sce.h)
#include DEVICE_FAMILY_PATH(inc/hw_aux_smph.h)
#include DEVICE_FAMILY_PATH(inc/hw_aux_evctl.h)
#include DEVICE_FAMILY_PATH(inc/hw_aux_aiodio.h)
#include DEVICE_FAMILY_PATH(inc/hw_aux_wuc.h)
#include DEVICE_FAMILY_PATH(inc/hw_event.h)
#include DEVICE_FAMILY_PATH(inc/hw_ints.h)
#include DEVICE_FAMILY_PATH(inc/hw_ioc.h)
#include <string.h>
#if defined(__IAR_SYSTEMS_ICC__)
    #include <intrinsics.h>
#endif


/// Driver internal data (located in MCU domain RAM, not shared with the Sensor Controller)
static SCIF_DATA_T scifData;


/// Import OSAL
#define SCIF_INCLUDE_OSAL_C_FILE
#include "scif_osal_none.c"


// Workaround for register field renaming
#ifndef AUX_WUC_MODCLKEN0_ANAIF_M
    #define AUX_WUC_MODCLKEN0_ANAIF_M AUX_WUC_MODCLKEN0_SOC_M
#endif




/// Task data structure buffer control: Size (in bytes)
#define SCIF_TASK_STRUCT_CTRL_SIZE                  (3 * sizeof(uint16_t))
/// Task data structure buffer control: Sensor Controller Engine's pointer negative offset (ref. struct start)
#define SCIF_TASK_STRUCT_CTRL_SCE_ADDR_BACK_OFFSET  (3 * sizeof(uint16_t))
/// Task data structure buffer control: Driver/MCU's pointer negative offset (ref. struct start)
#define SCIF_TASK_STRUCT_CTRL_MCU_ADDR_BACK_OFFSET  (2 * sizeof(uint16_t))




/** \brief Initializes a single I/O pin for Sensor Controller usage
  *
  * This function must be called for each I/O pin to be used after AUX I/O latching has been set
  * transparent. It configures (in the indicated order):
  * - AIODIO:
  *     - IOMODE
  *     - GPIODOUT
  *     - GPIODIE
  * - IOC:
  *     -IOCFGn (index remapped using \ref scifData.pAuxIoIndexToMcuIocfgOffsetLut[])
  *
  * \param[in]      auxIoIndex
  *     Index of the I/O pin, 0-15, using AUX mapping
  * \param[in]      ioMode
  *     Pin I/O mode, one of the following:
  *     - \ref AUXIOMODE_OUTPUT
  *     - \ref AUXIOMODE_INPUT
  *     - \ref AUXIOMODE_OPEN_DRAIN
  *     - \ref AUXIOMODE_OPEN_DRAIN_WITH_INPUT
  *     - \ref AUXIOMODE_OPEN_SOURCE
  *     - \ref AUXIOMODE_OPEN_SOURCE_WITH_INPUT
  *     - \ref AUXIOMODE_ANALOG
  * \param[in]      pullLevel
  *     Pull level to be used when the pin is configured as input, open-drain or open-source
  *     - No pull: -1
  *     - Pull-down: 0
  *     - Pull-up: 1
  * \param[in]      outputValue
  *     Initial output value when the pin is configured as output, open-drain or open-source
  */
void scifInitIo(uint32_t auxIoIndex, uint32_t ioMode, int pullLevel, uint32_t outputValue) {

    // Calculate access parameters from the AUX I/O index
    uint32_t auxAiodioBase = AUX_AIODIO0_BASE;
    uint32_t auxAiodioPin  = auxIoIndex;
    if (auxAiodioPin >= 8) {
        auxAiodioBase = AUX_AIODIO1_BASE;
        auxAiodioPin -= 8;
    }

    // Setup the AUX I/O controller
    HWREG(auxAiodioBase + AUX_AIODIO_O_IOMODE)   = (HWREG(auxAiodioBase + AUX_AIODIO_O_IOMODE)   & ~(0x03 << (2 * auxAiodioPin))) | (ioMode << (2 * auxAiodioPin));
    HWREG(auxAiodioBase + AUX_AIODIO_O_GPIODOUT) = (HWREG(auxAiodioBase + AUX_AIODIO_O_GPIODOUT) & ~(0x01 << (auxAiodioPin)))     | (outputValue << auxAiodioPin);
    HWREG(auxAiodioBase + AUX_AIODIO_O_GPIODIE)  = (HWREG(auxAiodioBase + AUX_AIODIO_O_GPIODIE)  & ~(0x01 << (auxAiodioPin)))     | ((ioMode >> 16) << auxAiodioPin);
    // Ensure that the settings have taken effect
    HWREG(auxAiodioBase + AUX_AIODIO_O_GPIODIE);

    // Configure pull level and transfer control of the I/O pin to AUX
    scifReinitIo(auxIoIndex, pullLevel);

} // scifInitIo




/** \brief Re-initializes a single I/O pin for Sensor Controller usage
  *
  * This function must be called after the AUX AIODIO has been initialized, or when reinitializing I/Os
  * that have been lent temporarily to MCU domain peripherals. It only configures the following:
  * - IOC:
  *     -IOCFGn (index remapped using \ref scifData.pAuxIoIndexToMcuIocfgOffsetLut[])
  *
  * \param[in]      auxIoIndex
  *     Index of the I/O pin, 0-15, using AUX mapping
  * \param[in]      pullLevel
  *     Pull level to be used when the pin is configured as input, open-drain or open-source
  *     - No pull: -1
  *     - Pull-down: 0
  *     - Pull-up: 1
  */
void scifReinitIo(uint32_t auxIoIndex, int pullLevel) {

    // Calculate access parameters from the AUX I/O index
    uint32_t mcuIocfgOffset = scifData.pAuxIoIndexToMcuIocfgOffsetLut[auxIoIndex];

    // Setup the MCU I/O controller, making the AUX I/O setup effective
    uint32_t iocfg = IOC_IOCFG0_PORT_ID_AUX_IO;
    switch (pullLevel) {
    case -1: iocfg |= IOC_IOCFG0_PULL_CTL_DIS; break;
    case 0:  iocfg |= IOC_IOCFG0_PULL_CTL_DWN; break;
    case 1:  iocfg |= IOC_IOCFG0_PULL_CTL_UP; break;
    }
    HWREG(IOC_BASE + IOC_O_IOCFG0 + mcuIocfgOffset) = iocfg;

} // scifReinitIo




/** \brief Uninitializes a single I/O pin after Sensor Controller usage
  *
  * This detaches the I/O pin from the AUX domain, and configures it as GPIO with input/output disabled
  * and the specified pull level.
  *
  * \param[in]      auxIoIndex
  *     Index of the I/O pin, 0-15, using AUX mapping
  * \param[in]      pullLevel
  *     Pull level
  *     - No pull: -1
  *     - Pull-down: 0
  *     - Pull-up: 1
  */
void scifUninitIo(uint32_t auxIoIndex, int pullLevel) {

    // Calculate access parameters from the AUX I/O index
    uint32_t mcuIocfgOffset = scifData.pAuxIoIndexToMcuIocfgOffsetLut[auxIoIndex];

    // Unconfigure the MCU I/O controller (revert to GPIO with input/output disabled and desired pull
    // level)
    uint32_t iocfg = IOC_IOCFG0_PORT_ID_GPIO;
    switch (pullLevel) {
    case -1: iocfg |= IOC_IOCFG0_PULL_CTL_DIS; break;
    case 0:  iocfg |= IOC_IOCFG0_PULL_CTL_DWN; break;
    case 1:  iocfg |= IOC_IOCFG0_PULL_CTL_UP; break;
    }
    HWREG(IOC_BASE + IOC_O_IOCFG0 + mcuIocfgOffset) = iocfg;

} // scifUninitIo




/** \brief Returns the index of the least significant '1' in the given bit-vector
  *
  * \param[in]      x
  *     The bit-vector
  *
  * \return
  *     The bit index of the least significant '1', for example 2 for 0x0004, or 32 if all bits are '0'
  */
static int scifFindLeastSignificant1(uint32_t x) {
#if defined(__IAR_SYSTEMS_ICC__) || defined(DOXYGEN)
    return __CLZ(__RBIT(x));
#elif defined(__TI_COMPILER_VERSION__) || defined(__CC_ARM)
    return __clz(__rbit(x));
#else
    return __builtin_ctz(x);
#endif
} // scifFindLeastSignificant1




/** \brief Initializes the driver
  *
  * This function must be called to enable the driver for operation. The function:
  * - Verifies that the driver is not already active
  * - Stores a local copy of the driver setup data structure, \ref SCIF_DATA_T
  * - Configures AUX domain hardware modules for operation (general and setup-specific). This includes
  *   complete I/O setup for all Sensor Controller tasks.
  * - Loads the generated AUX RAM image into the AUX RAM
  * - Initializes handshaking mechanisms for control, alert interrupt generation and data exchange
  * - Configures use of AUX domain wake-up sources
  * - Starts the Sensor Controller
  *
  * \param[in]      *pScifDriverSetup
  *     Driver setup, containing all relevant pointers and parameters for operation
  *
  * \return
  *     \ref SCIF_SUCCESS if successful, or \ref SCIF_ILLEGAL_OPERATION if the Sensor Controller already
  *     is active. The function call has no effect if unsuccessful.
  */
SCIF_RESULT_T scifInit(const SCIF_DATA_T* pScifDriverSetup) {
    uint32_t key;

    // Perform sanity checks: The Sensor Controller cannot already be active
    if (HWREG(AON_WUC_BASE + AON_WUC_O_AUXCTL) & AON_WUC_AUXCTL_SCE_RUN_EN_M) {
        return SCIF_ILLEGAL_OPERATION;
    }

    // Copy the driver setup
    memcpy(&scifData, pScifDriverSetup, sizeof(SCIF_DATA_T));

    // Enable clock for required AUX modules
    /*
    HWREG(AUX_WUC_BASE + AUX_WUC_O_MODCLKEN0) = AUX_WUC_MODCLKEN0_SMPH_M |
                                                AUX_WUC_MODCLKEN0_AIODIO0_M |
                                                AUX_WUC_MODCLKEN0_AIODIO1_M |
                                                AUX_WUC_MODCLKEN0_TIMER_M |
                                                AUX_WUC_MODCLKEN0_ANAIF_M |
                                                AUX_WUC_MODCLKEN0_TDC_M |
                                                AUX_WUC_MODCLKEN0_AUX_DDI0_OSC_M |
                                                AUX_WUC_MODCLKEN0_AUX_ADI4_M;
	*/
    // Open the AUX I/O latches, which have undefined value after power-up. AUX_AIODIO will by default
    // drive '0' on all I/O pins, so AUX_AIODIO must be configured before IOC
    HWREG(AUX_WUC_BASE + AUX_WUC_O_AUXIOLATCH) = AUX_WUC_AUXIOLATCH_EN_TRANSP;

    // Upload the AUX RAM image
    memcpy((void*) AUX_RAM_BASE, scifData.pAuxRamImage, scifData.auxRamImageSize);

    // Perform task resource initialization
    scifData.fptrTaskResourceInit();

    // Map events to the Sensor Controller's vector table, and set reset vector = AON wakeup
    HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_VECCFG0) = AUX_EVCTL_VECCFG0_VEC0_EV_AON_SW | AUX_EVCTL_VECCFG0_VEC0_EN_M |
    												 AUX_EVCTL_VECCFG0_VEC1_EV_AON_SW | AUX_EVCTL_VECCFG0_VEC1_EN_M;

    HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_VECCFG1) = AUX_EVCTL_VECCFG1_VEC2_EV_AON_SW |
                                                  AUX_EVCTL_VECCFG1_VEC3_EV_AON_SW | AUX_EVCTL_VECCFG1_VEC3_EN_M;

    HWREG(AUX_SCE_BASE + AUX_SCE_O_CTL) = 1 << AUX_SCE_CTL_RESET_VECTOR_S;

    // Clear any vector flags currently set (due to previous hardware or SCIF driver operation)
    HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_VECFLAGS) = 0x0000;

    // Set the READY event
    HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_SWEVSET) = AUX_EVCTL_SWEVSET_SWEV0_M;

    // Let AUX be powered down (clocks disabled, full retention) and the bus connection between the AUX
    // and MCU domains be disconnected by default. This may have been done already by the operating
    // system to be able to avoid framework dependencies on whether or not the Sensor Controller is used.
    HWREG(AUX_WUC_BASE + AUX_WUC_O_PWRDWNREQ) = 1;
    HWREG(AUX_WUC_BASE + AUX_WUC_O_MCUBUSCTL) = AUX_WUC_MCUBUSCTL_DISCONNECT_REQ;

    // Start the Sensor Controller, but first read a random register from the AUX domain to ensure
    // that the last write accesses have been completed
    HWREG(AUX_WUC_BASE + AUX_WUC_O_MCUBUSCTL);
    key = scifOsalEnterCriticalSection();
    HWREG(AON_WUC_BASE + AON_WUC_O_AUXCTL) |= AON_WUC_AUXCTL_SCE_RUN_EN_M;

    // The System CPU shall wake up on the ALERT event
    HWREG(AON_EVENT_BASE + AON_EVENT_O_MCUWUSEL) =
        (HWREG(AON_EVENT_BASE + AON_EVENT_O_MCUWUSEL) & ~(AON_EVENT_MCUWUSEL_WU0_EV_M << OSAL_MCUWUSEL_WU_EV_S)) |
        (AON_EVENT_MCUWUSEL_WU0_EV_AUX_SWEV1 << OSAL_MCUWUSEL_WU_EV_S);
    scifOsalLeaveCriticalSection(key);
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);

    // Register, clear and enable the interrupts
    osalRegisterCtrlReadyInt();
    osalClearCtrlReadyInt();
    osalEnableCtrlReadyInt();
    osalRegisterTaskAlertInt();
    osalClearTaskAlertInt();
    scifOsalEnableTaskAlertInt();

    return SCIF_SUCCESS;

} // scifInit




/** \brief Uninitializes the driver in order to release hardware resources or switch to another driver
  *
  * All Sensor Controller tasks must have been stopped before calling this function, to avoid leaving
  * hardware modules used in an unknown state. Also, any tick generation must have been stopped to avoid
  * leaving handshaking with the tick source in an unknown state.
  *
  * This function will wait until the Sensor Controller is sleeping before shutting it down.
  */
void scifUninit(void) {
    uint32_t key;

    // Wait until the Sensor Controller is idle (it might still be running, though not for long)
    while (!(HWREG(AUX_SCE_BASE + AUX_SCE_O_CPUSTAT) & AUX_SCE_CPUSTAT_SLEEP_M));

    // Stop and reset the Sensor Controller Engine
    HWREG(AUX_SCE_BASE + AUX_SCE_O_CTL) = AUX_SCE_CTL_RESTART_M;
    HWREG(AUX_SCE_BASE + AUX_SCE_O_CTL) = AUX_SCE_CTL_RESTART_M | AUX_SCE_CTL_SUSPEND_M;
    HWREG(AUX_SCE_BASE + AUX_SCE_O_CTL);
    key = scifOsalEnterCriticalSection();
    HWREG(AON_WUC_BASE + AON_WUC_O_AUXCTL) &= ~AON_WUC_AUXCTL_SCE_RUN_EN_M;
    scifOsalLeaveCriticalSection(key);
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
    HWREG(AUX_SCE_BASE + AUX_SCE_O_CTL) = 0x0000;

    // Disable interrupts
    osalDisableCtrlReadyInt();
    osalUnregisterCtrlReadyInt();
    scifOsalDisableTaskAlertInt();
    osalUnregisterTaskAlertInt();

    // Perform task resource uninitialization
    scifData.fptrTaskResourceUninit();

} // scifUninit




/** \brief Returns a bit-vector indicating the ALERT events associcated with the last ALERT interrupt
  *
  * This function shall be called by the application after it has received an ALERT interrupt, to find
  * which events have occurred.
  *
  * When all the alert events have been handled, the application must call \ref scifAckAlertEvents().
  * After acknowledging, this function must not be called again until the next ALERT event has been
  * received.
  *
  * \return
  *     The event bit-vector contains the following fields:
  *     - [15:8] Task input/output handling failed due to underflow/overflow, one bit per task ID
  *     - [7:0] Task input/output data exchange pending, one bit per task ID
  */
uint32_t scifGetAlertEvents(void) {
    return scifData.pTaskCtrl->bvTaskIoAlert;
} // scifGetAlertEvents




/** \brief Clears the ALERT interrupt source
  *
  * The application must call this function once and only once after reception of an ALERT interrupt,
  * before calling \ref scifAckAlertEvents().
  */
void scifClearAlertIntSource(void) {

    // Clear the source
    HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGSCLR) = AUX_EVCTL_EVTOAONFLAGS_SWEV1_M;

    // Ensure that the source clearing has taken effect
    while (HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGS) & AUX_EVCTL_EVTOAONFLAGS_SWEV1_M);

} // scifClearAlertIntSource




/** \brief Acknowledges the ALERT events associcated with the last ALERT interrupt
  *
  * This function shall be called after the handling the events associated with the last ALERT interrupt.
  *
  * The application must call this function once and only once after reception of an ALERT event,
  * after calling \ref scifClearAlertIntSource(). It must not be called unless an ALERT event has
  * occurred.
  *
  * \note Calling this function can delay (by a short period of time) the next task to be executed.
  */
void scifAckAlertEvents(void) {

    // Clear the events that have been handled now. This is needed for subsequent ALERT interrupts
    // generated by fwGenQuickAlertInterrupt(), since that procedure does not update bvTaskIoAlert.
    scifData.pTaskCtrl->bvTaskIoAlert = 0x0000;

    // Make sure that the CPU interrupt has been cleared before reenabling it
    osalClearTaskAlertInt();
    uint32_t key = scifOsalEnterCriticalSection();
    scifOsalEnableTaskAlertInt();

    // Set the ACK event to the Sensor Controller
    HWREGB(AUX_EVCTL_BASE + AUX_EVCTL_O_VECCFG1 + 1) = (AUX_EVCTL_VECCFG1_VEC3_EV_AON_SW | AUX_EVCTL_VECCFG1_VEC3_EN_M | AUX_EVCTL_VECCFG1_VEC3_POL_M) >> 8;
    HWREGB(AUX_EVCTL_BASE + AUX_EVCTL_O_VECCFG1 + 1) = (AUX_EVCTL_VECCFG1_VEC3_EV_AON_SW | AUX_EVCTL_VECCFG1_VEC3_EN_M) >> 8;
    scifOsalLeaveCriticalSection(key);

} // scifAckAlertEvents




/** \brief Selects whether or not the ALERT interrupt shall wake up the System CPU
  *
  * If the System CPU is in standby mode when the Sensor Controller generates an ALERT interrupt, the
  * System CPU will wake up by default.
  *
  * Call this function to disable or re-enable System CPU wake-up on ALERT interrupt. This can be used to
  * defer ALERT interrupt processing until the System CPU wakes up for other reasons, for example to
  * handle radio events, and thereby avoid unnecessary wake-ups.
  *
  * Note that there can be increased current consumption in System CPU standby mode if the ALERT
  * interrupt is disabled (by calling \ref scifOsalDisableTaskAlertInt()), but wake-up is enabled. This
  * is because the wake-up signal will remain asserted until \ref scifAckAlertEvents() has been called
  * for all pending ALERT events.
  *
  * The behavior resets to enabled when \ref scifInit() is called.
  *
  * \param[in]      enable
  *     Set to false to disable System CPU wake-up on ALERT interrupt, or true to reenable wake-up.
  */
void scifSetWakeOnAlertInt(bool enable) {
    uint32_t key = scifOsalEnterCriticalSection();
    uint32_t mcuwusel = HWREG(AON_EVENT_BASE + AON_EVENT_O_MCUWUSEL) & ~(AON_EVENT_MCUWUSEL_WU0_EV_M << OSAL_MCUWUSEL_WU_EV_S);
    if (enable) {
        scifData.pIntData->alertCanPdAuxMask = 0x0000;
        mcuwusel |= (AON_EVENT_MCUWUSEL_WU0_EV_AUX_SWEV1 << OSAL_MCUWUSEL_WU_EV_S);
    } else {
        scifData.pIntData->alertCanPdAuxMask = 0xFFFF;
        mcuwusel |= (AON_EVENT_MCUWUSEL_WU0_EV_NONE << OSAL_MCUWUSEL_WU_EV_S);
    }
    HWREG(AON_EVENT_BASE + AON_EVENT_O_MCUWUSEL) = mcuwusel;
    scifOsalLeaveCriticalSection(key);
} // scifSetWakeOnAlertInt




/** \brief Sets the initial task startup delay, in ticks
  *
  * This function may be used when starting multiple tasks at once, allowing for either:
  * - Spreading the execution times, for reduced peak current consumption and precise execution timing
  * - Aligning the execution times, for reduced total current consumption but less precise timing for
  *   lower-priority tasks
  *
  * If used, note the following:
  * - It replaces the call to \c fwScheduleTask() from the "Initialization Code"
  * - This function must be used with care when timer-based tasks are already running
  * - This function must always be called when starting the relevant tasks
  *
  * \param[in]      taskId
  *     ID of the task to set startup delay for
  * \param[in]      ticks
  *     Number of timer ticks until the first execution
  */
void scifSetTaskStartupDelay(uint32_t taskId, uint16_t ticks) {
    scifData.pTaskExecuteSchedule[taskId] = ticks;
} // scifSetTaskStartupDelay




/** \brief Resets the task data structures for the specified tasks
  *
  * This function must be called before tasks are restarted. The function resets the state data
  * structure, and optionally the \c input, \c output and \c state data structures.
  *
  * \param[in]      bvTaskIds
  *     Bit-vector indicating which tasks to reset (where bit N corresponds to task ID N)
  * \param[in]      bvTaskStructs
  *     Bit-vector indicating which task data structure types to reset in addition to \c state for these
  *     tasks. Make a bit-vector of \ref SCIF_STRUCT_CFG, \ref SCIF_STRUCT_INPUT and
  *     \ref SCIF_STRUCT_OUTPUT
  */
void scifResetTaskStructs(uint32_t bvTaskIds, uint32_t bvTaskStructs) {

    // Indicate that the data structure has been cleared
    scifData.bvDirtyTasks &= ~bvTaskIds;

    // Always clean the state data structure
    bvTaskStructs |= (1 << SCIF_STRUCT_STATE);

    // As long as there are more tasks to reset ...
    while (bvTaskIds) {
        uint32_t taskId = scifFindLeastSignificant1(bvTaskIds);
        bvTaskIds &= ~(1 << taskId);

        // For each data structure to be reset ...
        uint32_t bvStructs = bvTaskStructs;
        do {
            int n = scifFindLeastSignificant1(bvStructs);
            bvStructs &= ~(1 << n);
            uint32_t taskStructInfo = scifData.pTaskDataStructInfoLut[(taskId * 4) + n];

            // If it exists ...
            if (taskStructInfo) {

                // Split the information
                uint16_t addr   = (taskStructInfo >> 0)  & 0x0FFF; // 11:0
                uint16_t count  = (taskStructInfo >> 12) & 0x00FF; // 19:12
                uint16_t size   = (taskStructInfo >> 20) & 0x0FFF; // 31:20
                uint16_t length = sizeof(uint16_t) * size * count;

                // If multiple-buffered, include the control variables
                if (count > 1) {
                    addr   -= SCIF_TASK_STRUCT_CTRL_SIZE;
                    length += SCIF_TASK_STRUCT_CTRL_SIZE;
                }

                // Reset the data structure
                memcpy((void*) (AUX_RAM_BASE + addr), ((const uint8_t*) scifData.pAuxRamImage) + addr, length);
            }

        } while (bvStructs);
    }

} // scifResetTaskStructs




/** \brief Returns the number of input/output data buffers available for production/consumption
  *
  * When performing task data exchange with multiple-buffered data structures, the application calls this
  * function to get:
  * - The number of input buffers ready to be produced (\a taskStructType = \ref SCIF_STRUCT_INPUT)
  * - The number of output buffers ready to be consumed (\a taskStructType = \ref SCIF_STRUCT_OUTPUT)
  *
  * The application is only allowed to access the returned number of buffers, or less. The function
  * always returns 0 if a buffer overrun or underrun has occurred but has not yet been reported by
  * \ref scifGetAlertEvents(). For each buffer to be produced or consumed, the application must complete
  * these steps:
  * - Call \ref scifGetTaskStruct() to get a pointer to the data structure
  * - If input, populate the data structure. If output, process the data structure contents and reset
  *   contents manually, as needed.
  * - Call \ref scifHandoffTaskStruct() to give/return the buffer to the Sensor Controller Engine
  *
  * For single-buffered data structures, the function has no effect and always returns 0.
  *
  * \param[in]      taskId
  *     Task ID selection
  * \param[in]      taskStructType
  *     Task data structure type selection
  *
  * \return
  *     The number of buffers that can be produced/consumed by the application
  */
uint32_t scifGetTaskIoStructAvailCount(uint32_t taskId, SCIF_TASK_STRUCT_TYPE_T taskStructType) {

    // Fetch the information about the data structure
    uint32_t taskStructInfo = scifData.pTaskDataStructInfoLut[(taskId * 4) + (int) taskStructType];
    uint16_t baseAddr = (taskStructInfo >> 0)  & 0x0FFF; // 11:0
    uint16_t count    = (taskStructInfo >> 12) & 0x00FF; // 19:12
    uint16_t size     = (taskStructInfo >> 20) & 0x0FFF; // 31:20

    // If single-buffered, it's always 0
    if (count < 2) {
        return 0;
    }

    // Fetch the current memory addresses used by SCE and MCU
    uint16_t sceAddr = *((volatile uint16_t*) (AUX_RAM_BASE + baseAddr - SCIF_TASK_STRUCT_CTRL_SCE_ADDR_BACK_OFFSET));
    uint16_t mcuAddr = *((volatile uint16_t*) (AUX_RAM_BASE + baseAddr - SCIF_TASK_STRUCT_CTRL_MCU_ADDR_BACK_OFFSET));

    // Buffer overflow or underflow can occur in the background if the Sensor Controller produces or
    // consumes data too fast for the System CPU application. If this happens, return 0 so that the
    // application can detect the error by calling scifGetAlertEvents() in the next ALERT interrupt
    // before starting to process potentially corrupted or out-of-sync buffers.
    if (scifData.pIntData->bvTaskIoAlert & (0x0100 << taskId)) {
        return 0;
    }

    // Detect all buffers available
    // LSBs are different when none are available -> handled in the calculation further down
    if (mcuAddr == sceAddr) {
        return count;
    }

    // Calculate the number of buffers available
    mcuAddr &= ~0x0001;
    sceAddr &= ~0x0001;
    if (sceAddr < mcuAddr) {
        sceAddr += size * sizeof(uint16_t) * count;
    }
    return (sceAddr - mcuAddr) / (size * sizeof(uint16_t));

} // scifGetTaskIoStructAvailCount




/** \brief Returns a pointer to the specified data structure
  *
  * This function must be used to access multiple-buffered data structures, in which case it finds the
  * correct data structure buffer to be produced/consumed by the application. The application must use
  * \ref scifGetTaskIoStructAvailCount() to get the number of buffers to produce/consume.
  *
  * This function can also be used for single-buffered data structures, but this is less efficient than
  * accessing these data structures directly.
  *
  * \param[in]      taskId
  *     Task ID selection
  * \param[in]      taskStructType
  *     Task data structure type selection
  *
  * \return
  *     Pointer to the data structure (must be casted to correct pointer type)
  */
void* scifGetTaskStruct(uint32_t taskId, SCIF_TASK_STRUCT_TYPE_T taskStructType) {

    // Fetch the information about the data structure
    uint32_t taskStructInfo = scifData.pTaskDataStructInfoLut[(taskId * 4) + (int) taskStructType];
    uint16_t baseAddr = (taskStructInfo >> 0)  & 0x0FFF; // 11:0
    uint16_t count    = (taskStructInfo >> 12) & 0x00FF; // 19:12

    // If single-buffered, just return the base address
    if (count < 2) {
        return (void*) (AUX_RAM_BASE + baseAddr);

    // If multiple-buffered, return the MCU address
    } else {
        uint16_t mcuAddr = *((uint16_t*) (AUX_RAM_BASE + baseAddr - SCIF_TASK_STRUCT_CTRL_MCU_ADDR_BACK_OFFSET));
        return (void*) (AUX_RAM_BASE + (mcuAddr & ~0x0001));
    }

} // scifGetTaskStruct




/** \brief Called to handoff the an input or output data structure to the Sensor Controller Engine
  *
  * For output, this function shall be called after consuming a buffer of a multiple-buffered data
  * structure. This hands over the output buffer back to the Sensor Controller.
  *
  * For input, this function shall be called after producing a buffer of a multiple-buffered data
  * structure. This hands over the input buffer to the Sensor Controller.
  *
  * \param[in]      taskId
  *     Task ID selection
  * \param[in]      taskStructType
  *     Task data structure type selection
  */
void scifHandoffTaskStruct(uint32_t taskId, SCIF_TASK_STRUCT_TYPE_T taskStructType) {

    // Fetch the information about the data structure
    uint32_t taskStructInfo = scifData.pTaskDataStructInfoLut[(taskId * 4) + (int) taskStructType];
    uint16_t baseAddr = (taskStructInfo >> 0)  & 0x0FFF; // 11:0
    uint16_t count    = (taskStructInfo >> 12) & 0x00FF; // 19:12
    uint16_t size     = (taskStructInfo >> 20) & 0x0FFF; // 31:20

    // If multiple-buffered, move on the MCU address to the next buffer
    if (count >= 2) {

        // Move on the address
        uint16_t* pMcuAddr = (uint16_t*) (AUX_RAM_BASE + baseAddr - SCIF_TASK_STRUCT_CTRL_MCU_ADDR_BACK_OFFSET);
        uint16_t newMcuAddr = *pMcuAddr + size * sizeof(uint16_t);

        // If it has wrapped, move it back to the start and invert LSB
        if ((newMcuAddr & ~0x0001) > (baseAddr + (size * sizeof(uint16_t) * (count - 1)))) {
            newMcuAddr = baseAddr | ((newMcuAddr & 0x0001) ^ 0x0001);
        }

        // Write back the new address
        *pMcuAddr = newMcuAddr;
    }

} // scifHandoffTaskStruct




/** \brief Common function for manually starting, executing and terminating tasks
  *
  * \param[in]      bvTaskIds
  *     Bit-vector of task IDs for the tasks to be controlled
  * \param[in]      bvTaskReq
  *     Any legal combination of the following bits:
  *     - 0x01 : Starts the specified tasks
  *     - 0x02 : Executes the specified tasks once
  *     - 0x04 : Stops the specified tasks
  *
  * \return
  *     \ref SCIF_SUCCESS if successful, otherwise \ref SCIF_NOT_READY (last non-blocking call has not
  *     completed) or SCIF_ILLEGAL_OPERATION (attempted to execute an already active task). The function
  *     call has no effect if unsuccessful.
  */
static SCIF_RESULT_T scifCtrlTasksNbl(uint32_t bvTaskIds, uint32_t bvTaskReq) {

    // Prevent interruptions by concurrent scifCtrlTasksNbl() calls
    if (!osalLockCtrlTaskNbl()) {
        return SCIF_NOT_READY;
    }

    // Perform sanity checks: Starting already active or dirty tasks is illegal
    if (bvTaskReq & 0x01) {
        if ((scifData.pTaskCtrl->bvActiveTasks | scifData.bvDirtyTasks) & bvTaskIds) {
            osalUnlockCtrlTaskNbl();
            return SCIF_ILLEGAL_OPERATION;
        }
    }

    // Verify that the control interface is ready, then clear the READY interrupt source
    if (HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGS) & AUX_EVCTL_EVTOAONFLAGS_SWEV0_M) {
        HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGSCLR) = AUX_EVCTL_EVTOAONFLAGS_SWEV0_M;
        while (HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGS) & AUX_EVCTL_EVTOAONFLAGS_SWEV0_M);
    } else {
        osalUnlockCtrlTaskNbl();
        return SCIF_NOT_READY;
    }

    // Initialize tasks?
    if (bvTaskReq & 0x01) {
        scifData.pTaskCtrl->bvTaskInitializeReq = bvTaskIds;
        scifData.bvDirtyTasks |= bvTaskIds;
    } else {
        scifData.pTaskCtrl->bvTaskInitializeReq = 0x0000;
    }

    // Execute tasks?
    if (bvTaskReq & 0x02) {
        scifData.pTaskCtrl->bvTaskExecuteReq = bvTaskIds;
    } else {
        scifData.pTaskCtrl->bvTaskExecuteReq = 0x0000;
    }

    // Terminate tasks? Terminating already inactive tasks is allowed, because tasks may stop
    // spontaneously, and there's no way to know this for sure (it may for instance happen at any moment
    // while calling this function)
    if (bvTaskReq & 0x04) {
        scifData.pTaskCtrl->bvTaskTerminateReq = bvTaskIds;
    } else {
        scifData.pTaskCtrl->bvTaskTerminateReq = 0x0000;
    }

    // Make sure that the CPU interrupt has been cleared before reenabling it
    osalClearCtrlReadyInt();
    osalEnableCtrlReadyInt();

    // Set the REQ event to hand over the request to the Sensor Controller
    HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_VECCFG0) |= AUX_EVCTL_VECCFG0_VEC0_POL_M;
    HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_VECCFG0) &= ~AUX_EVCTL_VECCFG0_VEC0_POL_M;
    osalUnlockCtrlTaskNbl();
    return SCIF_SUCCESS;

} // scifCtrlTasksNbl




/** \brief Executes the specified tasks once, from an inactive state
  *
  * This triggers the Initialization, Execution and Termination code for each task ID specified in
  * \a bvTaskIds. All selected code is completed for one task before proceeding with the next task. The
  * control READY event is generated when the tasks have been executed.
  *
  * This function should not be used to execute a task that implements the event handler code, because
  * the execution method does not allow for running the Event Handler code.
  *
  * \note This function must not be called for already active tasks.
  *
  * \note Task control does not interrupt ongoing code execution on the Sensor Controller, but it has
  *       priority over other triggers/wake-up sources. Calling this function can therefore delay
  *       upcoming Sensor Controller activities such as RTC-based task execution and Event Handler code
  *       execution.
  *
  * \param[in]      bvTaskIds
  *     Bit-vector indicating which tasks should be executed (where bit N corresponds to task ID N)
  *
  * \return
  *     \ref SCIF_SUCCESS if successful, otherwise \ref SCIF_NOT_READY (last non-blocking call has not
  *     completed) or \ref SCIF_ILLEGAL_OPERATION (attempted to execute an already active task). The
  *     function call has no effect if unsuccessful.
  */
SCIF_RESULT_T scifExecuteTasksOnceNbl(uint16_t bvTaskIds) {
    return scifCtrlTasksNbl(bvTaskIds, 0x07);
} // scifExecuteTasksOnceNbl




/** \brief Starts the specified tasks
  *
  * This triggers the initialization code for each task ID specified in \a bvTaskIds. The READY event
  * is generated when the tasks have been started.
  *
  * \note This function must not be called for already active tasks.
  *
  * \note Task control does not interrupt ongoing code execution on the Sensor Controller, but it has
  *       priority over other triggers/wake-up sources. Calling this function can therefore delay
  *       upcoming Sensor Controller activities such as RTC-based task execution and Event Handler code
  *       execution.
  *
  * \param[in]      bvTaskIds
  *     Bit-vector indicating which tasks to be started (where bit N corresponds to task ID N)
  *
  * \return
  *     \ref SCIF_SUCCESS if successful, otherwise \ref SCIF_NOT_READY (last non-blocking call has not
  *     completed) or \ref SCIF_ILLEGAL_OPERATION (attempted to start an already active task). The
  *     function call has no effect if unsuccessful.
  */
SCIF_RESULT_T scifStartTasksNbl(uint16_t bvTaskIds) {
    return scifCtrlTasksNbl(bvTaskIds, 0x01);
} // scifStartTasksNbl




/** \brief Stops the specified tasks
  *
  * This triggers the termination code for each task ID specified in \a bvTaskIds. The READY event is
  * generated when the tasks have been stopped.
  *
  * \note Task control does not interrupt ongoing code execution on the Sensor Controller, but it has
  *       priority over other triggers/wake-up sources. Calling this function can therefore delay
  *       upcoming Sensor Controller activities such as RTC-based task execution and Event Handler code
  *       execution.
  *
  * \param[in]      bvTaskIds
  *     Bit-vector indicating which tasks to be stopped (where bit N corresponds to task ID N)
  *
  * \return
  *     \ref SCIF_SUCCESS if successful, otherwise \ref SCIF_NOT_READY (last non-blocking call has not
  *     completed). The function call has no effect if unsuccessful.
  */
SCIF_RESULT_T scifStopTasksNbl(uint16_t bvTaskIds) {
    return scifCtrlTasksNbl(bvTaskIds, 0x04);
} // scifStopTasksNbl




/** \brief Triggers manually the Execution code blocks for the specified tasks
  *
  * This triggers the Execution code for each task ID specified in \a bvTaskIds. The READY event
  * is generated when the Execution code has run for all the specified tasks.
  *
  * Calling this function does not interrupt any ongoing activities on the Sensor Controller, and does
  * not affect RTC-based task execution.
  *
  * \note This function should only be called for already active tasks.
  *
  * \note Task control does not interrupt ongoing code execution on the Sensor Controller, but it has
  *       priority over other triggers/wake-up sources. Calling this function can therefore delay
  *       upcoming Sensor Controller activities such as RTC-based task execution and Event Handler code
  *       execution.
  *
  * \param[in]      bvTaskIds
  *     Bit-vector indicating which tasks should be executed (where bit N corresponds to task ID N)
  *
  * \return
  *     \ref SCIF_SUCCESS if successful, otherwise \ref SCIF_NOT_READY (last non-blocking call has not
  *     completed) or \ref SCIF_ILLEGAL_OPERATION (attempted to execute an already active task). The
  *     function call has no effect if unsuccessful.
  */
SCIF_RESULT_T scifSwTriggerExecutionCodeNbl(uint16_t bvTaskIds) {
    return scifCtrlTasksNbl(bvTaskIds, 0x02);
} // scifSwTriggerExecutionCodeNbl




/** \brief Triggers manually the event handler code block
  *
  * This function forces execution of the Event Handler code, for whichever task it belongs to.
  *
  * Calling this function does not interrupt any ongoing activities on the Sensor Controller. It does
  * however cancel any previous trigger setup by a \c evhSetupCompbTrigger(), \c evhSetupGpioTrigger()
  * or \c evhSetupCompbTrigger() procedure call in task code.
  *
  * \note This function should only be called when the task using the event handler code is already
  *       active.
  */
void scifSwTriggerEventHandlerCode(void) {
    HWREGB(AUX_EVCTL_BASE + AUX_EVCTL_O_VECCFG1) = AUX_EVCTL_VECCFG1_VEC2_EV_AON_SW | AUX_EVCTL_VECCFG1_VEC2_EN_M | AUX_EVCTL_VECCFG1_VEC2_POL_M;
} // scifSwTriggerEventHandlerCode




/** \brief Waits for a non-blocking call to complete, with timeout
  *
  * The non-blocking task control functions, \ref scifExecuteTasksOnceNbl(), \ref scifStartTasksNbl()
  * and \ref scifStopTasksNbl(), may take some time to complete. This wait function can be used to make
  * blocking calls, and allow an operating system to switch context when until the task control interface
  * becomes ready again.
  *
  * The function returns when the last non-blocking call has completed, or immediately if already
  * completed. The function can also return immediately with the \ref SCIF_ILLEGAL_OPERATION error if
  * called from multiple threads of execution with non-zero \a timeoutUs.
  *
  * \b Important: Unlike the ALERT event, the READY event does not generate MCU domain and System CPU
  * wake-up. Depending on the SCIF OSAL implementation, this function might not return before the
  * specified timeout expires, even if the READY event has occurred long before that. To avoid such
  * delays, call \c fwGenAlertInterrupt() from the task code block that \ref scifWaitOnNbl() is waiting
  * for to complete.
  *
  * \param[in]      timeoutUs
  *     Maximum number of microseconds to wait for the non-blocking functions to become available. Use a
  *     timeout of "0" to check whether the interface already is available, or simply call the control
  *     function (which also will return \ref SCIF_NOT_READY if not ready).
  *
  * \return
  *     \ref SCIF_SUCCESS if the last call has completed, otherwise \ref SCIF_NOT_READY (the timeout
  *     expired) or \ref SCIF_ILLEGAL_OPERATION (the OSAL does not allow this function to be called with
  *     non-zero \a timeoutUs from multiple threads of execution).
  */
SCIF_RESULT_T scifWaitOnNbl(uint32_t timeoutUs) {
    if (HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGS) & AUX_EVCTL_EVTOAONFLAGS_SWEV0_M) {
        return SCIF_SUCCESS;
    } else {
        return osalWaitOnCtrlReady(timeoutUs);
    }
} // scifWaitOnNbl




/** \brief Returns a bit-vector indicating which tasks are currently active
  *
  * The bit-vector is maintained by the Sensor Controller. If called before the last non-blocking control
  * operation has completed, the bit-vector may indicate the task states as before or after the non-
  * blocking operations (the bit vector is updated only once per non-blocking call).
  *
  * \return
  *     A bit-vector indicating which tasks are active (bit N corresponds to task N)
  */
uint16_t scifGetActiveTaskIds(void) {
    return scifData.pTaskCtrl->bvActiveTasks;
} // scifGetActiveTaskIds


//@}


// Generated by WIN-4BBEDC4NT59 at 2018-01-20 21:46:25.572

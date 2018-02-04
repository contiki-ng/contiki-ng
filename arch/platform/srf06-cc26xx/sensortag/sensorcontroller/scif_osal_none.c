/// \addtogroup module_scif_osal
//@{
#ifdef SCIF_INCLUDE_OSAL_C_FILE


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
#include DEVICE_FAMILY_PATH(inc/hw_nvic.h)
#include DEVICE_FAMILY_PATH(driverlib/cpu.h)
#include DEVICE_FAMILY_PATH(driverlib/interrupt.h)
#include "scif_osal_none.h"


/// MCU wakeup source to be used with the Sensor Controller task ALERT event, must not conflict with OS
#define OSAL_MCUWUSEL_WU_EV_S   AON_EVENT_MCUWUSEL_WU3_EV_S


/// The READY interrupt is implemented using INT_AON_AUX_SWEV0
#define INT_SCIF_CTRL_READY     INT_AON_AUX_SWEV0
/// The ALERT interrupt is implemented using INT_AON_AUX_SWEV1
#define INT_SCIF_TASK_ALERT     INT_AON_AUX_SWEV1


/// Calculates the NVIC register offset for the specified interrupt
#define NVIC_OFFSET(i)          (((i) - 16) / 32)
/// Calculates the bit-vector to be written or compared against for the specified interrupt
#define NVIC_BV(i)              (1 << ((i - 16) % 32))


// Function prototypes
static void osalCtrlReadyIsr(void);
static void osalTaskAlertIsr(void);




/** \brief Registers the control READY interrupt
  *
  * This registers the \ref osalCtrlReadyIsr() function with the \ref INT_SCIF_CTRL_READY interrupt
  * vector.
  *
  * The interrupt occurs at initial startup, and then after each control request has been serviced by the
  * Sensor Controller. The interrupt is pending (including source event flag) and disabled while the task
  * control interface is idle.
  */
static void osalRegisterCtrlReadyInt(void) {
    IntRegister(INT_SCIF_CTRL_READY, osalCtrlReadyIsr);
} // osalRegisterCtrlReadyInt




/** \brief Unregisters the control READY interrupt
  *
  * This detaches the \ref osalCtrlReadyIsr() function from the \ref INT_SCIF_CTRL_READY interrupt
  * vector.
  */
static void osalUnregisterCtrlReadyInt(void) {
    IntUnregister(INT_SCIF_CTRL_READY);
} // osalUnregisterCtrlReadyInt




/** \brief Enables the control READY interrupt
  *
  * This function is called when sending a control REQ event to the Sensor Controller to enable the READY
  * interrupt. This is done after clearing the event source and then the READY interrupt, using
  * \ref osalClearCtrlReadyInt().
  */
static void osalEnableCtrlReadyInt(void) {
    HWREG(NVIC_EN0 + NVIC_OFFSET(INT_SCIF_CTRL_READY)) = NVIC_BV(INT_SCIF_CTRL_READY);
} // osalEnableCtrlReadyInt




/** \brief Disables the control READY interrupt
  *
  * This function is called when by the control READY ISR, \ref osalCtrlReadyIsr(), so that the interrupt
  * is disabled but still pending (including source event flag) while the task control interface is
  * idle/ready.
  */
static void osalDisableCtrlReadyInt(void) {
    HWREG(NVIC_DIS0 + NVIC_OFFSET(INT_SCIF_CTRL_READY)) = NVIC_BV(INT_SCIF_CTRL_READY);
} // osalDisableCtrlReadyInt




/** \brief Clears the task control READY interrupt
  *
  * This is done when sending a control request, after clearing the READY source event.
  */
static void osalClearCtrlReadyInt(void) {
    HWREG(NVIC_UNPEND0 + NVIC_OFFSET(INT_SCIF_CTRL_READY)) = NVIC_BV(INT_SCIF_CTRL_READY);
} // osalClearCtrlReadyInt




/** \brief Registers the task ALERT interrupt
  *
  * This registers the \ref osalTaskAlertIsr() function with the \ref INT_SCIF_TASK_ALERT interrupt
  * vector.
  *
  * The interrupt occurs whenever a sensor controller task alerts the driver, to request data exchange,
  * indicate an error condition or that the task has stopped spontaneously.
  */
static void osalRegisterTaskAlertInt(void) {
    IntRegister(INT_SCIF_TASK_ALERT, osalTaskAlertIsr);
} // osalRegisterTaskAlertInt




/** \brief Unregisters the task ALERT interrupt
  *
  * This detaches the \ref osalTaskAlertIsr() function from the \ref INT_SCIF_TASK_ALERT interrupt
  * vector.
  */
static void osalUnregisterTaskAlertInt(void) {
    IntUnregister(INT_SCIF_TASK_ALERT);
} // osalUnregisterTaskAlertInt




/** \brief Enables the task ALERT interrupt
  *
  * The interrupt is enabled at startup. It is disabled upon reception of a task ALERT interrupt and re-
  * enabled when the task ALERT is acknowledged.
  */
void scifOsalEnableTaskAlertInt(void) {
    HWREG(NVIC_EN0 + NVIC_OFFSET(INT_SCIF_TASK_ALERT)) = NVIC_BV(INT_SCIF_TASK_ALERT);
} // scifOsalEnableTaskAlertInt




/** \brief Disables the task ALERT interrupt
  *
  * The interrupt is enabled at startup. It is disabled upon reception of a task ALERT interrupt and re-
  * enabled when the task ALERT is acknowledged.
  *
  * Note that there can be increased current consumption in System CPU standby mode if the ALERT
  * interrupt is disabled, but wake-up is enabled (see \ref scifSetWakeOnAlertInt()). This is because the
  * wake-up signal will remain asserted until \ref scifAckAlertEvents() has been called for all pending
  * ALERT events.
  */
void scifOsalDisableTaskAlertInt(void) {
    HWREG(NVIC_DIS0 + NVIC_OFFSET(INT_SCIF_TASK_ALERT)) = NVIC_BV(INT_SCIF_TASK_ALERT);
} // scifOsalDisableTaskAlertInt




/** \brief Clears the task ALERT interrupt
  *
  * This is done when acknowledging the alert, after clearing the ALERT source event.
  */
static void osalClearTaskAlertInt(void) {
    HWREG(NVIC_UNPEND0 + NVIC_OFFSET(INT_SCIF_TASK_ALERT)) = NVIC_BV(INT_SCIF_TASK_ALERT);
} // osalClearTaskAlertInt




/** \brief Enters a critical section by disabling hardware interrupts
  *
  * \return
  *     Whether interrupts were enabled at the time this function was called
  */
uint32_t scifOsalEnterCriticalSection(void) {
    return !CPUcpsid();
} // scifOsalEnterCriticalSection




/** \brief Leaves a critical section by reenabling hardware interrupts if previously enabled
  *
  * \param[in]      key
  *     The value returned by the previous corresponding call to \ref scifOsalEnterCriticalSection()
  */
void scifOsalLeaveCriticalSection(uint32_t key) {
    if (key) CPUcpsie();
} // scifOsalLeaveCriticalSection




/// Stores whether task control non-blocking functions have been locked
static volatile bool osalCtrlTaskNblLocked = false;




/** \brief Locks use of task control non-blocking functions
  *
  * This function is used by the non-blocking task control to allow safe operation from multiple threads.
  *
  * The function shall attempt to set the \ref osalCtrlTaskNblLocked flag in a critical section.
  * Implementing a timeout is optional (the task control's non-blocking behavior is not associated with
  * this critical section, but rather with completion of the task control request).
  *
  * \return
  *     Whether the critical section could be entered (true if entered, false otherwise)
  */
static bool osalLockCtrlTaskNbl(void) {
    uint32_t key = !CPUcpsid();
    if (osalCtrlTaskNblLocked) {
        if (key) CPUcpsie();
        return false;
    } else {
        osalCtrlTaskNblLocked = true;
        if (key) CPUcpsie();
        return true;
    }
} // osalLockCtrlTaskNbl




/** \brief Unlocks use of task control non-blocking functions
  *
  * This function will be called once after a successful \ref osalLockCtrlTaskNbl().
  */
static void osalUnlockCtrlTaskNbl(void) {
    osalCtrlTaskNblLocked = false;
} // osalUnlockCtrlTaskNbl




/** \brief Waits until the task control interface is ready/idle
  *
  * This indicates that the task control interface is ready for the first request or that the last
  * request has been completed. If a timeout mechanisms is not available, the implementation may be
  * simplified.
  *
  * \note For the OSAL "None" implementation, a non-zero timeout corresponds to infinite timeout.
  *
  * \param[in]      timeoutUs
  *     Minimum timeout, in microseconds
  *
  * \return
  *     \ref SCIF_SUCCESS if the last call has completed, otherwise \ref SCIF_NOT_READY (the timeout
  *     expired) or \ref SCIF_ILLEGAL_OPERATION (the OSAL does not allow this function to be called with
  *     non-zero \a timeoutUs from multiple threads of execution).
  */
static SCIF_RESULT_T osalWaitOnCtrlReady(uint32_t timeoutUs) {
    if (HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGS) & AUX_EVCTL_EVTOAONFLAGS_SWEV0_M) {
        return SCIF_SUCCESS;
    } else if (timeoutUs) {
        while (!(HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGS) & AUX_EVCTL_EVTOAONFLAGS_SWEV0_M));
        return SCIF_SUCCESS;
    } else {
        return SCIF_NOT_READY;
    }
} // osalWaitOnCtrlReady




/// OSAL "None": Application-registered callback function for the task control READY interrupt
static SCIF_VFPTR osalIndicateCtrlReadyCallback = NULL;
/// OSAL "None": Application-registered callback function for the task ALERT interrupt
static SCIF_VFPTR osalIndicateTaskAlertCallback = NULL;




/** \brief Called by \ref osalCtrlReadyIsr() to notify the application
  *
  * This shall trigger a callback, generate a message/event etc.
  */
static void osalIndicateCtrlReady(void) {
    if (osalIndicateCtrlReadyCallback) {
        osalIndicateCtrlReadyCallback();
    }
} // osalIndicateCtrlReady




/** \brief Called by \ref osalTaskAlertIsr() to notify the application
  *
  * This shall trigger a callback, generate a message/event etc.
  */
static void osalIndicateTaskAlert(void) {
    if (osalIndicateTaskAlertCallback) {
        osalIndicateTaskAlertCallback();
    }
} // osalIndicateTaskAlert




/** \brief Sensor Controller READY interrupt service routine
  *
  * The ISR simply disables the interrupt and notifies the application.
  *
  * The interrupt flag is cleared and reenabled when sending the next task control request (by calling
  * \ref scifExecuteTasksOnceNbl(), \ref scifStartTasksNbl() or \ref scifStopTasksNbl()).
  */
static void osalCtrlReadyIsr(void) {
    osalDisableCtrlReadyInt();
    osalIndicateCtrlReady();
} // osalCtrlReadyIsr




/** \brief Sensor Controller ALERT interrupt service routine
  *
  * The ISR disables further interrupt generation and notifies the application. To clear the interrupt
  * source, the application must call \ref scifClearAlertIntSource. The CPU interrupt flag is cleared and
  * the interrupt is reenabled when calling \ref scifAckAlertEvents() to generate ACK.
  */
static void osalTaskAlertIsr(void) {
    scifOsalDisableTaskAlertInt();
    osalIndicateTaskAlert();
} // osalTaskAlertIsr




/** \brief OSAL "None": Registers the task control READY interrupt callback
  *
  * Using this callback is normally optional. See \ref osalIndicateCtrlReady() for details.
  *
  * \param[in]      callback
  *     Callback function pointer "void func(void)"
  */
void scifOsalRegisterCtrlReadyCallback(SCIF_VFPTR callback) {
    osalIndicateCtrlReadyCallback = callback;
} // scifOsalRegisterCtrlReadyCallback




/** \brief OSAL "None": Registers the task ALERT interrupt callback
  *
  * Using this callback is normally required. See \ref osalIndicateTaskAlert() for details.
  *
  * \param[in]      callback
  *     Callback function pointer "void func(void)"
  */
void scifOsalRegisterTaskAlertCallback(SCIF_VFPTR callback) {
    osalIndicateTaskAlertCallback = callback;
} // scifOsalRegisterTaskAlertCallback




/** \brief OSAL "None": Enables the AUX domain and Sensor Controller for access from the MCU domain
  *
  * This function must be called before accessing/using any of the following:
  * - Oscillator control registers
  * - AUX ADI registers
  * - AUX module registers and AUX RAM
  * - SCIF API functions, except \ref scifOsalEnableAuxDomainAccess()
  * - SCIF data structures
  *
  * The application is responsible for:
  * - Registering the last set access control state
  * - Ensuring that this control is thread-safe
  */
void scifOsalEnableAuxDomainAccess(void) {

    // Force on AUX domain clock and bus connection
    HWREG(AON_WUC_BASE + AON_WUC_O_AUXCTL) |= AON_WUC_AUXCTL_AUX_FORCE_ON_M;
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);

    // Wait for it to take effect
    while (!(HWREG(AON_WUC_BASE + AON_WUC_O_PWRSTAT) & AON_WUC_PWRSTAT_AUX_PD_ON_M));

} // scifOsalEnableAuxDomainAccess




/** \brief OSAL "None": Disables the AUX domain and Sensor Controller for access from the MCU domain
  *
  * The application is responsible for:
  * - Registering the last set access control state
  * - Ensuring that this control is thread-safe
  */
void scifOsalDisableAuxDomainAccess(void) {

    // Force on AUX domain bus connection
    HWREG(AON_WUC_BASE + AON_WUC_O_AUXCTL) &= ~AON_WUC_AUXCTL_AUX_FORCE_ON_M;
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);

    // Wait for it to take effect
    while (HWREG(AON_WUC_BASE + AON_WUC_O_PWRSTAT) & AON_WUC_PWRSTAT_AUX_PD_ON_M);

} // scifOsalDisableAuxDomainAccess


#endif
//@}


// Generated by WIN-4BBEDC4NT59 at 2018-01-20 21:46:25.572

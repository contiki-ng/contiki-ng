/** \addtogroup module_scif_generic_interface Generic Driver Interface
  *
  * \section section_usage Usage
  * A generic driver API and framework is used to control and exchange data with the Sensor Controller.
  * See below for guidelines on:
  * - \ref section_scif_init
  * - \ref section_scif_uninit
  * - \ref section_scif_aux_domain_access
  * - \ref section_scif_task_struct_access
  * - \ref section_scif_task_control
  * - \ref section_scif_usage_data_exchange
  *
  *
  * \subsection section_scif_init Initialization
  * The driver must be initialized before use by calling \ref scifInit(), with a pointer to the
  * \ref module_scif_driver_setup to be used, \ref SCIF_DATA_T. The call:
  * - Verifies that the driver is not already active
  * - Stores a local copy of the driver setup data structure, \ref SCIF_DATA_T
  * - Configures AUX domain hardware modules for operation (general and setup-specific). This includes
  *   complete I/O setup for all Sensor Controller tasks.
  * - Loads the generated AUX RAM image into the AUX RAM
  * - Initializes handshaking mechanisms for control, alert interrupt generation and data exchange
  * - Configures use of AUX domain wake-up sources
  * - Starts the Sensor Controller
  *
  * Additional initialization may be required depending on the selected \ref module_scif_osal and enabled
  * Sensor Controller task resources. For example, when using RTC-based task scheduling and TI-RTOS, the
  * following calls must be made:
  * \code
  * scifOsalInit();
  * scifOsalRegisterCtrlReadyCallback( ... );
  * scifOsalRegisterTaskAlertCallback( ... );
  * scifInit(&scifDriverSetup);
  * scifStartRtcTicksNow( ... );
  * \endcode
  *
  * If using RTC-based task scheduling, the application or OS is also required to start the RTC itself.
  * This can be done at any point, independently of calls to \ref scifInit().
  *
  *
  * \subsection section_scif_uninit Uninitialization and Driver Setup Switching
  * It is possible to have multiple Sensor Controller Interface driver setups in one application, however
  * these are not allowed to run at the same time. To switch from one driver setup to another, the
  * currently used driver setup must be deactivated first by calling \ref scifUninit().
  * \code
  * // Load the driver setup with source code prefix "ABC"
  * scifInit(&scifAbcDriverSetup);
  *
  * ...
  *
  * // Switch to the driver setup with source code prefix "XYZ"
  * scifUninit();
  * scifInit(&scifXyzDriverSetup);
  *
  * ...
  * \endcode
  *
  * Additional uninitialization may be required depending on the selected \ref module_scif_osal and
  * enabled Sensor Controller task resources.
  *
  *
  * \subsection section_scif_aux_domain_access AUX Domain Access
  * When the Sensor Controller is used (\ref scifInit() has been called), the AUX domain will by default
  * not have any clock while the Sensor Controller is idle. In this state, the System CPU and DMA will
  * not have access to the following AUX domain and SCIF resources:
  * - Oscillator control registers
  * - AUX ADI registers for ADC, analog comparators, current source etc.
  * - AUX sub-module registers and AUX RAM
  * - SCIF API functions
  * - SCIF data structures located in AUX RAM
  *
  * The implementation of AUX domain access control is OS-specific. For some OSAL implementations
  * (for example "TI-RTOS"), this is handled automatically by the OS framework so that the features
  * listed above are in practice always accessible to the application. For other OSAL implementations
  * (for example "None"), the application must call provided AUX domain access control functions whenever
  * it needs or no longer needs access to the AUX domain and SCIF features.
  *
  *
  * \subsection section_scif_task_struct_access Task Data Structure Access
  * The task data structures can be accessed in two ways:
  * - Directly through the task data structure instance (located in AUX RAM)
  * - Indirectly using \ref scifGetTaskStruct(), with task ID and data structure type as input
  *
  * The first method is more efficient, and be used with the \c cfg and \c state structures, and
  * with single-buffered \c input and \c output data structures.
  *
  * The second method can be used for any data structure, including multiple-buffered \c input and
  * \c output data structures, where it will select the correct buffer (to be accessed by the
  * application) automatically.
  *
  *
  * \subsection section_scif_task_control Task Control
  * Sensor Controller tasks are started, stopped or can be executed once using these generic functions:
  * - \ref scifStartTasksNbl() - Starts the specified tasks, by triggering their Initialization code
  *     - If needed, \ref scifSetTaskStartupDelay() can be used to skew the execution timing for the
  *       tasks started. This may also be used to spread or concentrate in time data exchange processing.
  * - \ref scifStopTasksNbl() - Stops the specified tasks, by cancelling any scheduled execution and
  *   running the Termination code.
  * - \ref scifExecuteTasksOnceNbl() - Triggers the Initialization, Execution and Termination code once
  *   for each specified task.
  *
  * The above functions are non-blocking in the sense that they do not wait for the Sensor Controller to
  * run the task code specified. To wait for the last non-blocking call to finish, or check if it has,
  * the application can call \ref scifWaitOnNbl(), or wait for the task control READY interrupt
  * (depending on the OSAL implementation). New non-blocking calls will fail until the last non-blocking
  * call has finished.
  *
  * To check which tasks are currently active, use \ref scifGetActiveTaskIds().
  *
  * Before starting a task, the application may be required to set parameters in the task's configuration
  * data structure. For example:
  * \code
  * scifTaskData.lightSensor.cfg.lowThreshold  = 42;
  * scifTaskData.lightSensor.cfg.highThreshold = 123;
  * scifStartTasksNbl(BV(SCIF_LIGHT_SENSOR_TASK_ID));
  * \endcode
  *
  * When restarting a task, the associated data structures will be modified, and the task data structures
  * will be marked as "dirty". To reset data structures and remove the "dirty" condition, call
  * \ref scifResetTaskStructs() to reset the data structure contents as needed (by copying the original
  * structure contents). Resetting the \c state data structure is mandatory.
  *
  *
  * \subsubsection section_scif_manual_trigger Manually Triggered Task Execution
  * For currently active Sensor Controller tasks, it is possible to trigger the Execution code and Event
  * Handler code manually from the System CPU application:
  * - \ref scifSwTriggerExecutionCodeNbl() - Triggers the Execution code once for each specified task.
  *   This function is non-blocking in the same way as \ref scifStartTasksNbl(), \ref scifStopTasksNbl()
  *   and \ref scifExecuteTasksOnceNbl(). New non-blocking calls will fail until the last non-blocking
  *   call has finished.
  * - \ref scifSwTriggerEventHandlerCode() - Triggers the Event Handler code (for whichever task it
  *   belongs to).
  *
  *
  * \subsubsection section_scif_task_control_multithreading Task Control in Multiple Threads of Execution
  * Follow these guidelines when controlling Sensor Controller tasks from multiple threads of execution:
  * - Always check the return value from every call to \ref scifStartTasksNbl(), \ref scifStopTasksNbl(),
  *   \ref scifExecuteTasksOnceNbl(), \ref scifSwTriggerExecutionCodeNbl() and \ref scifWaitOnNbl()
  * - Do one of the following if \ref scifWaitOnNbl() is called with non-zero timeout from multiple
  *   threads of execution:
  *     - Call \ref scifWaitOnNbl() only after a successful call to \ref scifStartTasksNbl(),
  *       \ref scifStopTasksNbl(), \ref scifExecuteTasksOnceNbl() or \ref scifSwTriggerExecutionCodeNbl(),
  *       in the same thread of execution
  *     - Use a semaphore or similar mechanism to ensure that only one thread of execution can perform
  *       task control operations at any time
  * - If possible, avoid controlling a single Sensor Controller task from multiple threads of execution.
  *   Otherwise, use a semaphore or similar mechanism to ensure that one sequence of task control
  *   operations cannot interrupt another.
  *
  *
  * \subsection section_scif_usage_data_exchange Data Exchange
  * Task data exchange is performed on the Sensor Controller's initiative when the task code calls:
  * - \c fwGenAlertInterrupt() or \c fwGenQuickAlertInterrupt() for single-buffered data exchange or
  *   other proprietary use
  * - \c fwSwitchOutputBuffer() for multiple-buffered output data exchange
  *
  * Calling either of these procedures triggers a task ALERT interrupt from the driver, which, depending
  * on the OSAL implementation, allows the application to consume/process generated output data or
  * produce/set new input data.
  *
  * When processing the ALERT interrupt, the application must:
  * - Call \ref scifClearAlertIntSource() to clear the interrupt source from AUX_EVCTL
  * - Call \ref scifGetAlertEvents() to determine which tasks have pending events. If there is only one
  *   task and there is no risk of buffer overflow or underflow, this can be skipped. For each task with
  *   pending events:
  *     - For single-buffered output data exchange:
  *         - Access the task's output data, either directly or indirectly using \ref scifGetTaskStruct()
  *     - For multiple-buffered output data exchange:
  *         - Call \ref scifGetTaskIoStructAvailCount() to get the number of buffers to be exchanged, and
  *           repeat the following steps the indicated number of times:
  *             - Call \ref scifGetTaskStruct() to get a pointer to the correct buffer
  *             - Access the data structure
  *             - Call \ref scifHandoffTaskStruct() to hand over the buffer to the Sensor Controller
  * - Call \ref scifAckAlertEvents() to acknowledge the handled events. If additional events have
  *   occurred in the meantime, the ALERT interrupt will be triggered again.
  *
  * \subsubsection section_scif_usage_interrupt_control Interrupt Control
  * Depending on the application it can desirable to defer System CPU wake-up on ALERT interrupt, for
  * example when using multiple-buffered output data exchange. It is also possible to disable ALERT
  * interrupt handling temporarily. This can be done using the following functions:
  * - Call \ref scifSetWakeOnAlertInt() to disable System CPU wake-up on ALERT interrupt, so that ALERT
  *   handling can be done when the System CPU wakes up for other reasons, for example to use the radio.
  * - Call \ref scifOsalDisableTaskAlertInt() to temporarily disable the ALERT interrupt handling on the
  *   System CPU. Note the following:
  *     - There can be increased current consumption in System CPU standby mode if the ALERT interrupt
  *       is disabled, but wake-up is enabled. This is because the wake-up signal will remain asserted
  *       until \ref scifAckAlertEvents() has been called for all pending ALERT events.
  *     - The SCIF framework itself disables the ALERT interrupt immediatly before generating the ALERT
  *       callback, and reenables it again in \ref scifAckAlertEvents().
  * - Call \ref scifOsalEnableTaskAlertInt() to reenable the ALERT interrupt handling on the System CPU.
  *
  * The ALERT interrupt and wake-up on ALERT interrupt are both enabled by default after calling
  * \ref scifInit().
  * @{
  */
#ifndef SCIF_FRAMEWORK_H
#define SCIF_FRAMEWORK_H

#include <stdint.h>
#include <stdbool.h>


/// Sensor Controller Interface function call result
typedef enum {
    SCIF_SUCCESS             = 0, ///< Call succeeded
    SCIF_NOT_READY           = 1, ///< Not ready (previous non-blocking call is still running)
    SCIF_ILLEGAL_OPERATION   = 2  ///< Illegal operation
} SCIF_RESULT_T;


/// Task data structure types
typedef enum {
    SCIF_STRUCT_CFG          = 0, ///< Task configuration data structure (Sensor Controller read-only)
    SCIF_STRUCT_INPUT        = 1, ///< Task input data structure
    SCIF_STRUCT_OUTPUT       = 2, ///< Task output data structure
    SCIF_STRUCT_STATE        = 3  ///< Task state data structure
} SCIF_TASK_STRUCT_TYPE_T;


/// Void function pointer type: "void func(void)"
typedef void (*SCIF_VFPTR)(void);


/// Sensor Controller internal data (located in AUX RAM)
#pragma pack(push, 2)
typedef struct {
    uint16_t taskId;              ///< ID of currently executed Sensor Controller task
    uint16_t bvTaskIoAlert;       ///< Pending input/output data alert (LSB = normal exchange, MSB = overflow or underflow)
    uint16_t alertGenMask;        ///< Can generate an ALERT interrupt now? (0xFFFF = yes, 0x0000 = no)
    uint16_t alertCanPdAuxMask;   ///< Can power down AUX domain after ALERT interrupt generation? (0xFFFF = yes, 0x0000 = no)
    uint16_t rtlStructLockReq;    ///< Run-time logging struct lock request (0=idle -> 1=requested -> 2=ready -> access -> 0=idle)
} SCIF_INT_DATA_T;
#pragma pack(pop)


/// Sensor Controller generic task control (located in AUX RAM)
#pragma pack(push, 2)
typedef struct {
    uint16_t bvActiveTasks;       ///< Indicates which tasks are currently active (only valid while ready)
    uint16_t bvTaskIoAlert;       ///< Input/output data alert (LSB = normal exchange, MSB = overflow or underflow)
    uint16_t bvTaskInitializeReq; ///< Requests tasks to start
    uint16_t bvTaskExecuteReq;    ///< Requests tasks to execute once immediately
    uint16_t bvTaskTerminateReq;  ///< Requests tasks to stop
} SCIF_TASK_CTRL_T;
#pragma pack(pop)


/// Driver internal data (located in main RAM, not shared with the Sensor Controller)
typedef struct {

    /// Sensor Controller internal data (located in AUX RAM)
    volatile SCIF_INT_DATA_T*   pIntData;
    /// Sensor Controller task generic control (located in AUX RAM)
    volatile SCIF_TASK_CTRL_T*  pTaskCtrl;
    /// Pointer to the task execution scheduling table
    volatile uint16_t*          pTaskExecuteSchedule;
    /// Bit-vector indicating tasks with potentially modified input/output/state data structures
    uint16_t                    bvDirtyTasks;

    /// AUX RAM image size (in bytes)
    uint16_t                    auxRamImageSize;
    /// AUX RAM image word array
    const uint16_t*             pAuxRamImage;

    /// Look-up table that converts from AUX I/O index to MCU IOCFG offset
    const uint32_t*             pTaskDataStructInfoLut;
    /// Look-up table of data structure information for each task
    const uint8_t*              pAuxIoIndexToMcuIocfgOffsetLut;

    /// Pointer to the project-specific hardware initialization function
    SCIF_VFPTR                  fptrTaskResourceInit;
    /// Pointer to the project-specific hardware uninitialization function
    SCIF_VFPTR                  fptrTaskResourceUninit;

    /// Pointer to the run-time logging task log request table
    volatile uint16_t*          pRtlTaskLogReq;
    /// Pointer to the run-time logging task log mask table
    volatile uint16_t*          pRtlTaskLogMask;
    /// Pointer to run-time logging data structure signatures (CRC-16)
    const uint16_t*             pRtlTaskStructSignatures;

} SCIF_DATA_T;


/// I/O pin mode: Output
#define AUXIOMODE_OUTPUT                        0x00000000
/// I/O pin mode: Input, active
#define AUXIOMODE_INPUT                         0x00010001
/// I/O pin mode: Input, inactive
#define AUXIOMODE_INPUT_IDLE                    0x00000001
/// I/O pin mode: Open drain (driven low, pulled high)
#define AUXIOMODE_OPEN_DRAIN                    0x00000002
/// I/O pin mode: Open drain + input (driven low, pulled high, input buffer enabled)
#define AUXIOMODE_OPEN_DRAIN_WITH_INPUT         0x00010002
/// I/O pin mode: Open source (driven high, pulled low)
#define AUXIOMODE_OPEN_SOURCE                   0x00000003
/// I/O pin mode: Open source + input (driven high, pulled low, input buffer enabled)
#define AUXIOMODE_OPEN_SOURCE_WITH_INPUT        0x00010003
/// I/O pin mode: Analog
#define AUXIOMODE_ANALOG                        0x00000001


// Prototypes for internal functions
void scifInitIo(uint32_t auxIoIndex, uint32_t ioMode, int pullLevel, uint32_t outputValue);
void scifReinitIo(uint32_t auxIoIndex, int pullLevel);
void scifUninitIo(uint32_t auxIoIndex, int pullLevel);


// Driver main control
SCIF_RESULT_T scifInit(const SCIF_DATA_T* pScifDriverSetup);
void scifUninit(void);

// Driver ALERT interrupt handling
uint32_t scifGetAlertEvents(void);
void scifClearAlertIntSource(void);
void scifAckAlertEvents(void);
void scifSetWakeOnAlertInt(bool wakeOnAlertInt);

// Task generic configuration functions
void scifSetTaskStartupDelay(uint32_t taskId, uint16_t ticks);

// Task data structure access functions
void scifResetTaskStructs(uint32_t bvTaskIds, uint32_t bvTaskStructs);
uint32_t scifGetTaskIoStructAvailCount(uint32_t taskId, SCIF_TASK_STRUCT_TYPE_T taskStructType);
void* scifGetTaskStruct(uint32_t taskId, SCIF_TASK_STRUCT_TYPE_T taskStructType);
void scifHandoffTaskStruct(uint32_t taskId, SCIF_TASK_STRUCT_TYPE_T taskStructType);

// Task control functions (non-blocking)
SCIF_RESULT_T scifExecuteTasksOnceNbl(uint16_t bvTaskIds);
SCIF_RESULT_T scifStartTasksNbl(uint16_t bvTaskIds);
SCIF_RESULT_T scifStopTasksNbl(uint16_t bvTaskIds);
SCIF_RESULT_T scifWaitOnNbl(uint32_t timeoutUs);

// Task code manual trigger functions
SCIF_RESULT_T scifSwTriggerExecutionCodeNbl(uint16_t bvTaskIds);
void scifSwTriggerEventHandlerCode(void);

// Task status functions
uint16_t scifGetActiveTaskIds(void);


#endif
//@}


// Generated by WIN-4BBEDC4NT59 at 2018-01-20 21:46:25.572

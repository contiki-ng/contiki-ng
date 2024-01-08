/**************************************************************************//**
 * @file     dpm.h
 * @version  V3.00
 * @brief    Debug Protection Mechanism (DPM) driver header file
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __DPM_H__
#define __DPM_H__

#ifdef __cplusplus
extern "C"
{
#endif


/** @addtogroup Standard_Driver Standard Driver
  @{
*/

/** @addtogroup DPM_Driver DPM Driver
  @{
*/

/** @addtogroup DPM_EXPORTED_CONSTANTS DPM Exported Constants
  @{
*/


/*---------------------------------------------------------------------------------------------------------*/
/* DPM Control Register Constant Definitions                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define SECURE_DPM          0   /*!< Secure DPM module */
#define NONSECURE_DPM       1   /*!< Non-secure DPM module */

#define DPM_CTL_WVCODE      (0x5AUL<<DPM_CTL_WVCODE_Pos)    /*!< Secure DPM control register write verify code */
#define DPM_CTL_RVCODE      (0xA5UL<<DPM_CTL_RVCODE_Pos)    /*!< Secure DPM control register read verify code */
#define DPM_NSCTL_WVCODE    (0x5AUL<<DPM_CTL_WVCODE_Pos)    /*!< Non-secure DPM control register write verify code */
#define DPM_NSCTL_RVCODE    (0xA5UL<<DPM_CTL_RVCODE_Pos)    /*!< Non-secure DPM control register read verify code */

/*---------------------------------------------------------------------------------------------------------*/
/* DPM Time-out Handler Constant Definitions                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define DPM_TIMEOUT         (SystemCoreClock)   /*!< 1 second time-out */
#define DPM_TIMEOUT_ERR     (-1L)               /*!< DPM time-out error value */

/**@}*/ /* end of group WDT_EXPORTED_CONSTANTS */

extern int32_t g_DPM_i32ErrCode;

/** @addtogroup DPM_EXPORTED_FUNCTIONS DPM Exported Functions
  @{
*/

/**
  * @brief      Wait DPM_STS Busy Flag
  * @param      None
  * @return     None
  * @details    This macro waits DPM_STS busy flag is cleared and skips when time-out.
  * @note       This macro sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
#define DPM_WAIT_STS_BUSY() \
    do{ \
        uint32_t u32TimeOutCnt = DPM_TIMEOUT; \
        g_DPM_i32ErrCode = 0; \
        while(DPM->STS & DPM_STS_BUSY_Msk) \
        { \
            if(--u32TimeOutCnt == 0) \
            { \
                g_DPM_i32ErrCode = DPM_TIMEOUT_ERR; \
                break; \
            } \
        } \
    }while(0)

/**
  * @brief      Wait DPM_NSSTS Busy Flag
  * @param      None
  * @return     None
  * @details    This macro waits DPM_NSSTS busy flag is cleared and skips when time-out.
  * @note       This macro sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
#define DPM_WAIT_NSSTS_BUSY() \
    do{ \
        uint32_t u32TimeOutCnt = DPM_TIMEOUT; \
        g_DPM_i32ErrCode = 0; \
        while(DPM->NSSTS & DPM_NSSTS_BUSY_Msk) \
        { \
            if(--u32TimeOutCnt == 0) \
            { \
                g_DPM_i32ErrCode = DPM_TIMEOUT_ERR; \
                break; \
            } \
        } \
    }while(0)

/**
  * @brief      Enable DPM Interrupt
  * @param      None
  * @return     None
  * @details    This macro enables DPM interrupt.
  *             This macro is for Secure DPM and Secure region only.
  * @note       This macro sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
#define DPM_ENABLE_INT() \
    do{ \
        DPM_WAIT_STS_BUSY(); \
        if(g_DPM_i32ErrCode == 0) \
            DPM->CTL = (DPM->CTL & (~DPM_CTL_WVCODE_Msk)) | (DPM_CTL_WVCODE|DPM_CTL_INTEN_Msk); \
    }while(0)

/**
  * @brief      Disable DPM Interrupt
  * @param      None
  * @return     None
  * @details    This macro disables DPM interrupt.
  *             This macro is for Secure DPM and Secure region only.
  * @note       This macro sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
#define DPM_DISABLE_INT() \
    do{ \
        DPM_WAIT_STS_BUSY(); \
        if(g_DPM_i32ErrCode == 0) \
            DPM->CTL = (DPM->CTL & (~(DPM_CTL_WVCODE_Msk|DPM_CTL_INTEN_Msk))) | (DPM_CTL_WVCODE); \
    }while(0)

/**
  * @brief      Enable Debugger to Access DPM Registers
  * @param      None
  * @return     None
  * @details    This macro enables debugger to access Secure and Non-secure DPM registers.
  *             This macro is for Secure DPM and Secure region only.
  * @note       This macro sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
#define DPM_ENABLE_DBG_ACCESS() \
    do{ \
        DPM_WAIT_STS_BUSY(); \
        if(g_DPM_i32ErrCode == 0) \
            DPM->CTL = (DPM->CTL & (~(DPM_CTL_WVCODE_Msk|DPM_CTL_DACCDIS_Msk))) | (DPM_CTL_WVCODE); \
    }while(0)

/**
  * @brief      Disable Debugger to Access DPM Registers
  * @param      None
  * @return     None
  * @details    This macro disables debugger to access Secure and Non-secure DPM registers.
  *             This macro is for Secure DPM and Secure region only.
  * @note       This macro sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
#define DPM_DISABLE_DBG_ACCESS() \
    do{ \
        DPM_WAIT_STS_BUSY(); \
        if(g_DPM_i32ErrCode == 0) \
            DPM->CTL = (DPM->CTL & (~DPM_CTL_WVCODE_Msk)) | (DPM_CTL_WVCODE|DPM_CTL_DACCDIS_Msk); \
    }while(0)


void DPM_SetDebugDisable(uint32_t u32dpm);
void DPM_SetDebugLock(uint32_t u32dpm);
int32_t DPM_GetDebugDisable(uint32_t u32dpm);
int32_t DPM_GetDebugLock(uint32_t u32dpm);
int32_t DPM_SetPasswordUpdate(uint32_t u32dpm, uint32_t au32Pwd[]);
int32_t DPM_SetPasswordCompare(uint32_t u32dpm, uint32_t au32Pwd[]);
int32_t DPM_GetPasswordErrorFlag(uint32_t u32dpm);
int32_t DPM_GetIntFlag(void);
void DPM_ClearPasswordErrorFlag(uint32_t u32dpm);
void DPM_EnableDebuggerWriteAccess(uint32_t u32dpm);
void DPM_DisableDebuggerWriteAccess(uint32_t u32dpm);



/**@}*/ /* end of group DPM_EXPORTED_FUNCTIONS */

/**@}*/ /* end of group DPM_Driver */

/**@}*/ /* end of group Standard_Driver */

#ifdef __cplusplus
}
#endif

#endif /* __DPM_H__ */

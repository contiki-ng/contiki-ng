/**************************************************************************//**
 * @file     dpm.c
 * @version  V3.00
 * @brief    Debug Protection Mechanism (DPM) driver source file
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "NuMicro.h"


/** @addtogroup Standard_Driver Standard Driver
  @{
*/

/** @addtogroup DPM_Driver DPM Driver
  @{
*/

int32_t g_DPM_i32ErrCode = 0; /*!< DPM global error code */

/** @addtogroup DPM_EXPORTED_FUNCTIONS DPM Exported Functions
  @{
*/

/**
  * @brief      Set Debug Disable
  * @param[in]  u32dpm  The pointer of the specified DPM module
  *                     - \ref SECURE_DPM
  *                     - \ref NONSECURE_DPM
  * @return     None
  * @details    This function sets Secure or Non-secure DPM debug disable.
  *             The debug disable function works after reset (chip reset or pin reset).
  * @note       This function sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
void DPM_SetDebugDisable(uint32_t u32dpm)
{
    DPM_T *dpm;

    if(__PC()&NS_OFFSET) dpm = DPM_NS;
    else dpm = DPM;

    if(u32dpm == SECURE_DPM) /* Secure DPM */
    {
        DPM_WAIT_STS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            dpm->CTL = (DPM->CTL & (~DPM_CTL_WVCODE_Msk)) | (DPM_CTL_WVCODE | DPM_CTL_DBGDIS_Msk);
    }
    else    /* Non-secure DPM */
    {
        DPM_WAIT_NSSTS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            dpm->NSCTL = (dpm->NSCTL & (~DPM_NSCTL_WVCODE_Msk)) | (DPM_NSCTL_WVCODE | DPM_NSCTL_DBGDIS_Msk);
    }
}

/**
  * @brief      Set Debug Lock
  * @param[in]  u32dpm  Select DPM module. Valid values are:
  *                     - \ref SECURE_DPM
  *                     - \ref NONSECURE_DPM
  * @return     None
  * @details    This function sets Secure or Non-secure DPM debug lock.
  *             The debug lock function works after reset (chip reset or pin reset).
  * @note       This function sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
void DPM_SetDebugLock(uint32_t u32dpm)
{
    DPM_T *dpm;

    if(__PC()&NS_OFFSET) dpm = DPM_NS;
    else dpm = DPM;

    if(u32dpm == SECURE_DPM) /* Secure DPM */
    {
        DPM_WAIT_STS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            dpm->CTL = (dpm->CTL & (~DPM_CTL_WVCODE_Msk)) | (DPM_CTL_WVCODE | DPM_CTL_LOCK_Msk);
    }
    else    /* Non-secure DPM */
    {
        DPM_WAIT_NSSTS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            dpm->NSCTL = (dpm->NSCTL & (~DPM_NSCTL_WVCODE_Msk)) | (DPM_NSCTL_WVCODE | DPM_NSCTL_LOCK_Msk);
    }
}

/**
  * @brief      Get Debug Disable
  * @param[in]  u32dpm  Select DPM module. Valid values are:
  *                     - \ref SECURE_DPM
  *                     - \ref NONSECURE_DPM
  * @retval     0   Debug is not in disable status.
  * @retval     1   Debug is in disable status.
  * @retval     -1  DPM time-our error
  * @details    This function gets Secure or Non-secure DPM debug disable status.
  *             If Secure debug is disabled, debugger cannot access Secure region and can access Non-secure region only.
  *             If Non-secure debug is disabled, debugger cannot access all Secure and Non-secure region.
  * @note       This function sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
int32_t DPM_GetDebugDisable(uint32_t u32dpm)
{
    int32_t i32RetVal = DPM_TIMEOUT_ERR;
    DPM_T *dpm;

    if(__PC()&NS_OFFSET) dpm = DPM_NS;
    else dpm = DPM;

    if(u32dpm == SECURE_DPM) /* Secure DPM */
    {
        DPM_WAIT_STS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            i32RetVal = (dpm->STS & DPM_STS_DBGDIS_Msk) >> DPM_STS_DBGDIS_Pos;
    }
    else    /* Non-secure DPM */
    {
        DPM_WAIT_NSSTS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            i32RetVal = (dpm->NSSTS & DPM_NSSTS_DBGDIS_Msk) >> DPM_NSSTS_DBGDIS_Pos;
    }

    return i32RetVal;
}

/**
  * @brief      Get Debug Lock
  * @param[in]  u32dpm  Select DPM module. Valid values are:
  *                     - \ref SECURE_DPM
  *                     - \ref NONSECURE_DPM
  * @retval     0   Debug is not in lock status.
  * @retval     1   Debug is in lock status.
  * @retval     -1  DPM time-our error
  * @details    This function gets Secure or Non-secure DPM debug disable status.
  *             If Secure debug is locked, debugger cannot access Secure region and can access Non-secure region only.
  *             If Non-secure debug is locked, debugger cannot access all Secure and Non-secure region.
  * @note       This function sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
int32_t DPM_GetDebugLock(uint32_t u32dpm)
{
    int32_t i32RetVal = DPM_TIMEOUT_ERR;
    DPM_T *dpm;

    if(__PC()&NS_OFFSET) dpm = DPM_NS;
    else dpm = DPM;

    if(u32dpm == SECURE_DPM) /* Secure DPM */
    {
        DPM_WAIT_STS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            i32RetVal = (dpm->STS & DPM_STS_LOCK_Msk) >> DPM_STS_LOCK_Pos;
    }
    else                    /* Non-secure DPM */
    {
        DPM_WAIT_NSSTS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            i32RetVal = (dpm->NSSTS & DPM_NSSTS_LOCK_Msk) >> DPM_NSSTS_LOCK_Pos;
    }

    return i32RetVal;
}

/**
  * @brief      Update DPM Password
  * @param[in]  u32dpm        Select DPM module. Valid values are:
  *                           - \ref SECURE_DPM
  *                           - \ref NONSECURE_DPM
  * @param[in]  au32Password  Password length is 256 bits.
  * @retval     0   No password is updated. The password update count has reached the maximum value.
  * @retval     1   Password update is successful.
  * @retval     -1  DPM time-our error
  * @details    This function updates Secure or Non-secure DPM password.
  * @note       This function sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
int32_t DPM_SetPasswordUpdate(uint32_t u32dpm, uint32_t au32Pwd[])
{
    uint32_t u32i;
    int32_t i32RetVal = DPM_TIMEOUT_ERR;
    DPM_T *dpm;

    if(__PC()&NS_OFFSET) dpm = DPM_NS;
    else dpm = DPM;

    if(u32dpm == SECURE_DPM) /* Secure DPM */
    {
        /* Set Secure DPM password */
        for(u32i = 0; u32i < 4; u32i++)
        {
            DPM_WAIT_STS_BUSY();
            if(g_DPM_i32ErrCode == 0)
                dpm->SPW[u32i] = au32Pwd[u32i];
            else break;
        }

        /* Set Secure DPM password update */
        if(g_DPM_i32ErrCode == 0)
        {
            DPM_WAIT_STS_BUSY();
            if(g_DPM_i32ErrCode == 0)
                dpm->CTL = (dpm->CTL & (~DPM_CTL_WVCODE_Msk)) | (DPM_CTL_WVCODE | DPM_CTL_PWUPD_Msk);
        }

        /* Check Secure DPM password update flag */
        if(g_DPM_i32ErrCode == 0)
        {
            DPM_WAIT_STS_BUSY();
            if(g_DPM_i32ErrCode == 0)
                i32RetVal = (dpm->STS & DPM_STS_PWUOK_Msk) >> DPM_STS_PWUOK_Pos;
        }

        /* Clear Secure DPM password update flag */
        if(g_DPM_i32ErrCode == 0)
        {
            DPM_WAIT_STS_BUSY();
            if( (i32RetVal == 1) && (g_DPM_i32ErrCode == 0) )
                dpm->STS = DPM_STS_PWUOK_Msk;
        }
    }
    else    /* Non-secure DPM */
    {
        /* Set Non-secure DPM password */
        for(u32i = 0; u32i < 4; u32i++)
        {
            DPM_WAIT_NSSTS_BUSY();
            if(g_DPM_i32ErrCode == 0)
                dpm->NSPW[u32i] = au32Pwd[u32i];
            else break;
        }

        /* Set Non-secure DPM password update */
        if(g_DPM_i32ErrCode == 0)
        {
            DPM_WAIT_NSSTS_BUSY();
            if(g_DPM_i32ErrCode == 0)
                dpm->NSCTL = (dpm->NSCTL & (~DPM_CTL_WVCODE_Msk)) | (DPM_CTL_WVCODE | DPM_NSCTL_PWUPD_Msk);          
        }

        /* Check Non-secure DPM password update flag */
        if(g_DPM_i32ErrCode == 0)
        {
            DPM_WAIT_NSSTS_BUSY();
            if(g_DPM_i32ErrCode == 0)
                i32RetVal = (dpm->NSSTS & DPM_NSSTS_PWUOK_Msk) >> DPM_NSSTS_PWUOK_Pos;            
        }

        /* Clear Non-secure DPM password update flag */
        if(g_DPM_i32ErrCode == 0)
        {
            DPM_WAIT_NSSTS_BUSY();
            if( (i32RetVal == 1) && (g_DPM_i32ErrCode == 0) )
                dpm->NSSTS = DPM_NSSTS_PWUOK_Msk;
        }
    }

    return i32RetVal;
}

/**
  * @brief      Compare DPM Password
  * @param[in]  u32dpm  Select DPM module. Valid values are:
  *                     - \ref SECURE_DPM
  *                     - \ref NONSECURE_DPM
  * @retval     0   The password comparison can be proccessed.
  * @retval     1   No more password comparison can be proccessed. \n
  *                 The password comparison fail times has reached the maximum value.
  * @retval     -1  DPM time-our error
  * @details    This function sets Secure or Non-secure DPM password comparison. \n
  *             The comparison result is checked by DPM_GetPasswordErrorFlag().
  * @note       This function sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
int32_t DPM_SetPasswordCompare(uint32_t u32dpm, uint32_t au32Pwd[])
{
    uint32_t u32i;
    int32_t i32RetVal = DPM_TIMEOUT_ERR;
    DPM_T *dpm;

    if(__PC()&NS_OFFSET) dpm = DPM_NS;
    else dpm = DPM;

    if(u32dpm == SECURE_DPM) /* Secure DPM */
    {
        /* Check Secure DPM password compare fail times maximum flag */
        if(dpm->STS & DPM_STS_PWFMAX_Msk)
        {
            i32RetVal = 1;
        }
        else
        {
            /* Set Secure DPM password */
            for(u32i = 0; u32i < 4; u32i++)
            {
                DPM_WAIT_STS_BUSY();
                if(g_DPM_i32ErrCode == 0)
                    dpm->SPW[u32i] = au32Pwd[u32i];
                else break;
            }

            /* Set Secure DPM password cpmpare */
            if(g_DPM_i32ErrCode == 0)
            {
                DPM_WAIT_STS_BUSY();
                if(g_DPM_i32ErrCode == 0)
                {
                    dpm->CTL = (dpm->CTL & (~DPM_CTL_WVCODE_Msk)) | (DPM_CTL_WVCODE | DPM_CTL_PWCMP_Msk);
                    i32RetVal = 0;
                }
            }
        }
    }
    else    /* Non-secure DPM */
    {
        /* Check Non-secure DPM password compare fail times maximum flag */
        if(dpm->NSSTS & DPM_NSSTS_PWFMAX_Msk)
        {
            i32RetVal = 1;
        }
        else
        {
            /* Set Non-secure DPM password */
            for(u32i = 0; u32i < 4; u32i++)
            {
                DPM_WAIT_NSSTS_BUSY();
                if(g_DPM_i32ErrCode == 0)
                    dpm->NSPW[u32i] = au32Pwd[u32i];
                else break;
            }

            /* Set Non-secure DPM password compare */
            if(g_DPM_i32ErrCode == 0)
            {
                DPM_WAIT_NSSTS_BUSY();
                if(g_DPM_i32ErrCode == 0)
                {
                    dpm->NSCTL = (dpm->NSCTL & (~DPM_NSCTL_WVCODE_Msk)) | (DPM_NSCTL_WVCODE | DPM_NSCTL_PWCMP_Msk);
                    i32RetVal = 0;
                }
            }
        }
    }

    return i32RetVal;
}

/**
  * @brief      Get DPM Password Error Flag
  * @param[in]  u32dpm        Select DPM module. Valid values are:
  *                           - \ref SECURE_DPM
  *                           - \ref NONSECURE_DPM
  * @retval     0   Specified DPM module password compare error flag is 0.
  * @retval     1   Specified DPM module password compare error flag is 1.
  * @retval     -1  DPM time-our error
  * @details    This function returns Secure or Non-secure DPM password compare error flag.
  * @note       This function sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
int32_t DPM_GetPasswordErrorFlag(uint32_t u32dpm)
{
    int32_t i32RetVal = DPM_TIMEOUT_ERR;
    DPM_T *dpm;

    if(__PC()&NS_OFFSET) dpm = DPM_NS;
    else dpm = DPM;

    if(u32dpm == SECURE_DPM) /* Secure DPM */
    {
        /* Check Secure DPM password compare error flag */
        DPM_WAIT_STS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            i32RetVal = (dpm->STS & DPM_STS_PWCERR_Msk) >> DPM_STS_PWCERR_Pos;
    }
    else    /* Non-secure DPM */
    {
        /* Check Non-secure DPM password compare error flag */
        DPM_WAIT_NSSTS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            i32RetVal = (dpm->NSSTS & DPM_NSSTS_PWCERR_Msk) >> DPM_NSSTS_PWCERR_Pos;
    }

    return i32RetVal;
}

/**
  * @brief      Get DPM Interrupt Flag
  * @param      None
  * @retval     0   Secure DPM interrupt flag is 0.
  * @retval     1   Secure DPM interrupt flag is 1.
  * @retval     -1  DPM time-our error
  * @details    This function returns Secure DPM interrupt flag.
  *             Secure DPM interrupt flag includes Secure and Non-secure DPM password compare error flag.
  *             This function is for Secure DPM and Secure region only.
  * @note       This function sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
int32_t DPM_GetIntFlag(void)
{
    int32_t i32RetVal = DPM_TIMEOUT_ERR;

    DPM_WAIT_STS_BUSY();
    if(g_DPM_i32ErrCode == 0)
        i32RetVal = (DPM->STS & DPM_STS_INT_Msk) >> DPM_STS_INT_Pos;

    return i32RetVal;
}


/**
  * @brief      Clear DPM Password Error Flag
  * @param[in]  u32dpm        Select DPM module. Valid values are:
  *                           - \ref SECURE_DPM
  *                           - \ref NONSECURE_DPM
  * @return     None
  * @details    This function clears Secure or Non-secure DPM password compare error flag.
  * @note       This function sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
void DPM_ClearPasswordErrorFlag(uint32_t u32dpm)
{
    DPM_T *dpm;

    if(__PC()&NS_OFFSET) dpm = DPM_NS;
    else dpm = DPM;

    if(u32dpm == SECURE_DPM) /* Secure DPM */
    {
        DPM_WAIT_STS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            dpm->STS = DPM_STS_PWCERR_Msk;
    }
    else    /* Non-secure DPM */
    {
        DPM_WAIT_NSSTS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            dpm->NSSTS = DPM_NSSTS_PWCERR_Msk;
    }
}

/**
  * @brief      Enable Debugger Write Access
  * @param[in]  u32dpm        Select DPM module. Valid values are:
  *                           - \ref SECURE_DPM
  *                           - \ref NONSECURE_DPM
  * @return     None
  * @details    This function enables external debugger to write Secure or Non-secure DPM registers.
  * @note       This function sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
void DPM_EnableDebuggerWriteAccess(uint32_t u32dpm)
{
    DPM_T *dpm;

    if(__PC()&NS_OFFSET) dpm = DPM_NS;
    else dpm = DPM;

    if(u32dpm == SECURE_DPM) /* Secure DPM */
    {
        DPM_WAIT_STS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            dpm->CTL = (dpm->CTL & (~(DPM_CTL_RVCODE_Msk | DPM_CTL_DACCWDIS_Msk))) | DPM_CTL_WVCODE;
    }
    else    /* Non-secure DPM */
    {
        DPM_WAIT_NSSTS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            dpm->NSCTL = (dpm->NSCTL & (~(DPM_NSCTL_RVCODE_Msk | DPM_NSCTL_DACCWDIS_Msk))) | DPM_NSCTL_WVCODE;
    }
}

/**
  * @brief      Disable Debugger Write Access
  * @param[in]  u32dpm        Select DPM module. Valid values are:
  *                           - \ref SECURE_DPM
  *                           - \ref NONSECURE_DPM
  * @return     None.
  * @details    This function disables external debugger to write Secure or Non-secure DPM registers.
  * @note       This function sets g_DPM_i32ErrCode to DPM_TIMEOUT_ERR if waiting DPM time-out.
  */
void DPM_DisableDebuggerWriteAccess(uint32_t u32dpm)
{
    DPM_T *dpm;

    if(__PC()&NS_OFFSET) dpm = DPM_NS;
    else dpm = DPM;

    if(u32dpm == SECURE_DPM) /* Secure DPM */
    {
        DPM_WAIT_STS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            dpm->CTL = (dpm->CTL & (~DPM_CTL_RVCODE_Msk)) | (DPM_CTL_WVCODE | DPM_CTL_DACCWDIS_Msk);
    }
    else    /* Non-secure DPM */
    {
        DPM_WAIT_NSSTS_BUSY();
        if(g_DPM_i32ErrCode == 0)
            dpm->NSCTL = (dpm->NSCTL & (~DPM_NSCTL_RVCODE_Msk)) | (DPM_NSCTL_WVCODE | DPM_NSCTL_DACCWDIS_Msk);
    }
}


/**@}*/ /* end of group DPM_EXPORTED_FUNCTIONS */

/**@}*/ /* end of group DPM_Driver */

/**@}*/ /* end of group Standard_Driver */
